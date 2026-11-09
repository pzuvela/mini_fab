#include <gtest/gtest.h>
#include <httplib.h>

#include <chrono>
#include <string>
#include <thread>
#include <algorithm>
#include <vector>

#include "controller/mini_fab_controller.h"
#include "persistence/sqlite_recipe_repository.h"
#include "recipes/recipe_manager.h"
#include "web/routes/wafer_routes.h"

using namespace std::chrono_literals;

class WaferRoutesTest : public ::testing::Test
{
protected:
    SqliteRecipeRepository repository_{":memory:"};
    RecipeManager manager_{repository_};
    MiniFabController controller_{manager_};

    httplib::Server server_;
    std::thread serverThread_;
    int port_ = -1;

    void SetUp() override
    {
        MiniFab::Web::registerWaferRoutes(
            server_, controller_
        );

        port_ = server_.bind_to_any_port("127.0.0.1");
        ASSERT_GT(port_, 0);

        serverThread_ = std::thread([this]()
        {
            server_.listen_after_bind();
        });

        for (int i = 0; i < 100 && !server_.is_running(); ++i)
        {
            std::this_thread::sleep_for(10ms);
        }

        ASSERT_TRUE(server_.is_running());
    }

    void TearDown() override
    {
        controller_.stopScheduler();
        server_.stop();

        if (serverThread_.joinable())
            serverThread_.join();
    }

    httplib::Client client() const
    {
        return httplib::Client("127.0.0.1", port_);
    }
};

TEST_F(WaferRoutesTest, ReturnsInitialWafer)
{
    auto cli = client();
    auto response = cli.Get("/api/wafers");

    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 200);

    EXPECT_EQ(
        response->body,
        R"({"wafers":[{"id":"W1","location":{"station":"LoadPort","slot":3}}]})"
    );
}

TEST_F(WaferRoutesTest, ReturnsJsonContentType)
{
    auto cli = client();
    auto response = cli.Get("/api/wafers");

    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 200);

    EXPECT_EQ(
        response->get_header_value("Content-Type"),
        "application/json"
    );
}

TEST_F(WaferRoutesTest, ReturnsRegisteredWafers)
{
    const Wafer wafer{
        "W2",
        {Station::LoadPort, 4}
    };

    ASSERT_EQ(controller_.registerWafer(wafer), WaferRegistrationResult::Registered);

    auto cli = client();
    auto response = cli.Get("/api/wafers");

    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 200);

    // unordered_map iteration order is not guaranteed.
    EXPECT_NE(
        response->body.find(R"("id":"W1")"),
        std::string::npos
    );

    EXPECT_NE(
        response->body.find(R"("id":"W2")"),
        std::string::npos
    );

    EXPECT_EQ(controller_.wafersSnapshot().size(), 2);
}

TEST_F(WaferRoutesTest, EscapesSpecialCharacters)
{
    const Wafer wafer{
        "W\"2",
        {Station::LoadPort, 4}
    };

    ASSERT_EQ(controller_.registerWafer(wafer), WaferRegistrationResult::Registered);

    auto cli = client();
    auto response = cli.Get("/api/wafers");

    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 200);

    EXPECT_NE(
        response->body.find(R"("id":"W\"2")"),
        std::string::npos
    );
}

TEST_F(WaferRoutesTest, ReflectsInventoryChanges)
{
    auto cli = client();

    auto before = cli.Get("/api/wafers");

    ASSERT_TRUE(before);
    EXPECT_EQ(
        before->body,
        R"({"wafers":[{"id":"W1","location":{"station":"LoadPort","slot":3}}]})"
    );

    ASSERT_EQ(controller_.registerWafer(Wafer{"W2", {Station::LoadPort, 4}}), WaferRegistrationResult::Registered);

    auto after = cli.Get("/api/wafers");

    ASSERT_TRUE(after);
    EXPECT_EQ(after->status, 200);

    EXPECT_NE(
        after->body.find(R"("id":"W2")"),
        std::string::npos
    );

    EXPECT_NE(before->body, after->body);
}



TEST_F(WaferRoutesTests, PostRegistersNewWafer)
{
    const auto response = client_.Post(
        "/api/wafers",
        R"({"id":"W2","station":"LoadPort","slot":4})",
        "application/json"
    );

    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 201);

    const auto wafers = controller_.wafersSnapshot();

    const auto found = std::find_if(
        wafers.begin(),
        wafers.end(),
        [](const Wafer& wafer)
        {
            return wafer.id == "W2";
        }
    );

    ASSERT_NE(found, wafers.end());
    EXPECT_EQ(found->location.station, Station::LoadPort);
    EXPECT_EQ(found->location.slot, 4);
}



TEST_F(WaferRoutesTests, PostRejectsDuplicateWaferId)
{
    const auto response = client_.Post(
        "/api/wafers",
        R"({"id":"W1","station":"LoadPort","slot":4})",
        "application/json"
    );

    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 409);
    EXPECT_NE(
        response->body.find("Wafer ID already exists"),
        std::string::npos
    );
}



TEST_F(WaferRoutesTests, PostRejectsOccupiedSlot)
{
    const auto response = client_.Post(
        "/api/wafers",
        R"({"id":"W2","station":"LoadPort","slot":3})",
        "application/json"
    );

    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 409);
    EXPECT_NE(
        response->body.find("Slot already occupied"),
        std::string::npos
    );
}


TEST_F(WaferRoutesTests, PostRejectsInvalidRequests)
{
    const std::vector<std::string> invalidRequests = {
        R"({"id":"W2","station":"LoadPort"})",
        R"({"id":"W2","station":"LoadPort","slot":"4"})",
        R"({"id":"W2","station":"LoadPort","slot":4,})",
        R"({"id":"W2","station":"LoadPort","slot":4.5})",
        R"({"id":"","station":"LoadPort","slot":4})",
        R"({"id":"W2","station":"LoadPort","slot":0})",
        R"({"id":"W2","station":"LoadPort","slot":-1})",
        R"({"id":"W2","station":"LoadPort","slot":4,"extra":true})",
        "not json"
    };

    for (const auto& json : invalidRequests)
    {
        SCOPED_TRACE(json);

        const auto response = client_.Post(
            "/api/wafers",
            json,
            "application/json"
        );

        ASSERT_TRUE(response);
        EXPECT_EQ(response->status, 400);
    }
}


TEST_F(WaferRoutesTests, RejectedRegistrationDoesNotModifyInventory)
{
    const auto before = controller_.wafersSnapshot();

    const auto response = client_.Post(
        "/api/wafers",
        R"({"id":"W2","station":"LoadPort","slot":3})",
        "application/json"
    );

    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 409);

    const auto after = controller_.wafersSnapshot();

    EXPECT_EQ(after.size(), before.size());
}

