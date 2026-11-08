
#include <gtest/gtest.h>
#include <httplib.h>

#include <chrono>
#include <string>
#include <thread>

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

