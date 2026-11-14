#include <gtest/gtest.h>
#include <httplib.h>

#include <chrono>
#include <string>
#include <thread>
#include <algorithm>
#include <vector>
#include <atomic>

#include "controller/mini_fab_controller.h"
#include "persistence/sqlite_recipe_repository.h"
#include "recipes/recipe_manager.h"
#include "web/routes/wafer_routes.h"
#include "persistence/sqlite_event_repository.h"
#include "persistence/sqlite_wafer_repository.h"


using namespace std::chrono_literals;

class WaferRoutesTests : public ::testing::Test
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

TEST_F(WaferRoutesTests, ReturnsInitialWafer)
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

TEST_F(WaferRoutesTests, ReturnsJsonContentType)
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

TEST_F(WaferRoutesTests, ReturnsRegisteredWafers)
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

TEST_F(WaferRoutesTests, EscapesSpecialCharacters)
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

TEST_F(WaferRoutesTests, ReflectsInventoryChanges)
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
    auto cli = client();

    const auto response = cli.Post(
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
    auto cli = client();

    const auto response = cli.Post(
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
    auto cli = client();

    const auto response = cli.Post(
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
        R"({"id":"W2","station":"LoadPort","slot":4,"extra":true})",
        "not json"
    };

    auto cli = client();

    for (const auto& json : invalidRequests)
    {
        SCOPED_TRACE(json);

        const auto response = cli.Post(
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
    auto cli = client();
    const auto response = cli.Post(
        "/api/wafers",
        R"({"id":"W2","station":"LoadPort","slot":3})",
        "application/json"
    );

    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 409);

    const auto after = controller_.wafersSnapshot();

    EXPECT_EQ(after.size(), before.size());
}


TEST_F(WaferRoutesTests, RejectsUnsupportedContentType)
{
    auto cli = client();

    const auto response = cli.Post(
        "/api/wafers",
        R"({"id":"W2","station":"LoadPort","slot":4})",
        "text/plain"
    );

    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 415);
    EXPECT_EQ(controller_.wafersSnapshot().size(), 1);
}

TEST_F(WaferRoutesTests, RejectsIntegerOverflow)
{
    auto cli = client();

    const auto response = cli.Post(
        "/api/wafers",
        R"({"id":"W2","station":"LoadPort","slot":999999999999999999999})",
        "application/json"
    );

    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 400);
    EXPECT_EQ(controller_.wafersSnapshot().size(), 1);
}

TEST_F(WaferRoutesTests, RejectsTrailingJsonContent)
{
    auto cli = client();

    const auto response = cli.Post(
        "/api/wafers",
        R"({"id":"W2","station":"LoadPort","slot":4} garbage)",
        "application/json"
    );

    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 400);
    EXPECT_EQ(controller_.wafersSnapshot().size(), 1);
}

TEST_F(WaferRoutesTests, RejectsInvalidWaferSemantics)
{
    auto cli = client();

    const std::vector<std::string> invalid = {
        R"({"id":"","station":"LoadPort","slot":4})",
        R"({"id":"W2","station":"LoadPort","slot":0})",
        R"({"id":"W2","station":"LoadPort","slot":-1})",
        R"({"id":"W2","station":"ProcessModule","slot":1})"
    };

    for (const auto& body : invalid)
    {
        SCOPED_TRACE(body);

        const auto response = cli.Post(
            "/api/wafers",
            body,
            "application/json"
        );

        ASSERT_TRUE(response);
        EXPECT_EQ(response->status, 422);
        EXPECT_EQ(controller_.wafersSnapshot().size(), 1);
    }
}

TEST_F(WaferRoutesTests, SuccessReturnsRegisteredWafer)
{
    auto cli = client();

    const auto response = cli.Post(
        "/api/wafers",
        R"({"id":"W2","station":"LoadPort","slot":4})",
        "application/json"
    );

    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 201);

    EXPECT_NE(
        response->body.find(R"("id":"W2")"),
        std::string::npos
    );

    EXPECT_NE(
        response->body.find(R"("station":"LoadPort")"),
        std::string::npos
    );

    EXPECT_NE(
        response->body.find(R"("slot":4)"),
        std::string::npos
    );
}


TEST_F(WaferRoutesTests, ConcurrentRegistrationSameId)
{
    std::atomic<bool> start{false};
    int statuses[2] = {0, 0};

    const std::string bodies[2] = {
        R"({"id":"W2","station":"LoadPort","slot":4})",
        R"({"id":"W2","station":"LoadPort","slot":5})"
    };

    auto registerWafer = [&](int index)
    {
        auto cli = client();

        while (!start.load(std::memory_order_acquire))
            std::this_thread::yield();

        const auto response = cli.Post(
            "/api/wafers",
            bodies[index],
            "application/json"
        );

        statuses[index] = response ? response->status : 0;
    };

    std::thread first(registerWafer, 0);
    std::thread second(registerWafer, 1);

    start.store(true, std::memory_order_release);

    first.join();
    second.join();

    std::sort(std::begin(statuses), std::end(statuses));

    EXPECT_EQ(statuses[0], 201);
    EXPECT_EQ(statuses[1], 409);

    const auto wafers = controller_.wafersSnapshot();

    ASSERT_EQ(wafers.size(), 2);

    const auto count = std::count_if(
        wafers.begin(),
        wafers.end(),
        [](const Wafer& wafer)
        {
            return wafer.id == "W2";
        }
    );

    EXPECT_EQ(count, 1);
}



TEST_F(WaferRoutesTests, ConcurrentRegistrationSameSlot)
{
    std::atomic<bool> start{false};
    int statuses[2] = {0, 0};

    const std::string bodies[2] = {
        R"({"id":"W2","station":"LoadPort","slot":4})",
        R"({"id":"W3","station":"LoadPort","slot":4})"
    };

    auto registerWafer = [&](int index)
    {
        auto cli = client();

        while (!start.load(std::memory_order_acquire))
            std::this_thread::yield();

        const auto response = cli.Post(
            "/api/wafers",
            bodies[index],
            "application/json"
        );

        statuses[index] = response ? response->status : 0;
    };

    std::thread first(registerWafer, 0);
    std::thread second(registerWafer, 1);

    start.store(true, std::memory_order_release);

    first.join();
    second.join();

    std::sort(std::begin(statuses), std::end(statuses));

    EXPECT_EQ(statuses[0], 201);
    EXPECT_EQ(statuses[1], 409);

    const auto wafers = controller_.wafersSnapshot();

    ASSERT_EQ(wafers.size(), 2);

    const auto count = std::count_if(
        wafers.begin(),
        wafers.end(),
        [](const Wafer& wafer)
        {
            return wafer.location.station == Station::LoadPort
                && wafer.location.slot == 4;
        }
    );

    EXPECT_EQ(count, 1);
}


TEST_F(WaferRoutesTests, RecoverWaferRejectsInvalidJson)
{
    auto cli = client();

    auto response = cli.Post(
        "/api/wafers/recover",
        R"({"waferId":"W1","destinationSlot":)",
        "application/json"
    );

    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 400);
    EXPECT_EQ(
        response->get_header_value("Content-Type"),
        "application/json"
    );
}

TEST_F(WaferRoutesTests, RecoverWaferRejectsMissingFields)
{
    auto cli = client();

    auto response = cli.Post(
        "/api/wafers/recover",
        R"({"waferId":"W2"})",
        "application/json"
    );

    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 400);
}

TEST_F(WaferRoutesTests, RecoverWaferRejectsUnsupportedContentType)
{
    auto cli = client();

    auto response = cli.Post(
        "/api/wafers/recover",
        R"({"waferId":"W2","destinationSlot":4})",
        "text/plain"
    );

    ASSERT_TRUE(response);

    EXPECT_EQ(response->status, 415);
    EXPECT_EQ(
        response->get_header_value("Content-Type"),
        "application/json"
    );
}


TEST(WaferRecoveryRoutesTests, RecoversStrandedWaferViaHttp)
{
    SqliteRecipeRepository recipeRepo(":memory:");
    SqliteEventRepository eventRepo(":memory:");
    SqliteWaferRepository waferRepo(":memory:");

    RecipeManager recipeManager(recipeRepo);

    // Simulate a wafer stranded in PM1.
    ASSERT_TRUE(waferRepo.insert({
        "W2", {Station::LoadPort, 4}
    }));

    ASSERT_TRUE(waferRepo.updateLocation(
        "W2", {Station::ProcessModule, 1}
    ));

    MiniFabController controller(
        recipeManager,
        eventRepo,
        waferRepo
    );

    httplib::Server server;

    MiniFab::Web::registerWaferRoutes(
        server, controller
    );

    const int port =
        server.bind_to_any_port("127.0.0.1");

    ASSERT_GT(port, 0);

    std::thread serverThread([&]()
    {
        server.listen_after_bind();
    });

    // Ensure the server is ready.
    for (int i = 0;
         i < 100 && !server.is_running();
         ++i)
    {
        std::this_thread::sleep_for(10ms);
    }

    httplib::Client client("127.0.0.1", port);

    auto response = client.Post(
        "/api/wafers/recover",
        R"({"waferId":"W2","destinationSlot":4})",
        "application/json"
    );

    // Always clean up the server before assertions.
    server.stop();
    serverThread.join();

    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 200);

    const auto wafers = controller.wafersSnapshot();

    const auto it = std::find_if(
        wafers.begin(),
        wafers.end(),
        [](const Wafer& wafer)
        {
            return wafer.id == "W2";
        }
    );

    ASSERT_NE(it, wafers.end());

    EXPECT_EQ(
        it->location,
        (Location{Station::LoadPort, 4})
    );

    EXPECT_FALSE(
        controller.status().processModuleWaferId.has_value()
    );

    EXPECT_EQ(
        controller.status().state,
        EquipmentState::Idle
    );

    // Verify persistence, not just in-memory state.
    const auto persisted = waferRepo.getAll();

    const auto persistedIt = std::find_if(
        persisted.begin(),
        persisted.end(),
        [](const Wafer& wafer)
        {
            return wafer.id == "W2";
        }
    );

    ASSERT_NE(persistedIt, persisted.end());

    EXPECT_EQ(
        persistedIt->location,
        (Location{Station::LoadPort, 4})
    );
}


TEST_F(WaferRoutesTests, RecoveryRejectsMalformedRequests)
{
    const std::vector<std::string> invalidRequests = {
        // Trailing comma
        R"({"waferId":"W2","destinationSlot":4,})",

        // Duplicate field
        R"({"waferId":"W2","waferId":"W3","destinationSlot":4})",

        // Invalid type
        R"({"waferId":"W2","destinationSlot":"4"})"
    };

    auto cli = client();

    for (const auto& body : invalidRequests)
    {
        SCOPED_TRACE(body);

        const auto before = controller_.wafersSnapshot();

        const auto response = cli.Post(
            "/api/wafers/recover",
            body,
            "application/json"
        );

        ASSERT_TRUE(response);
        EXPECT_EQ(response->status, 400);
        EXPECT_EQ(
            response->get_header_value("Content-Type"),
            "application/json"
        );

        const auto after = controller_.wafersSnapshot();
        EXPECT_EQ(after.size(), before.size());
        EXPECT_EQ(
            controller_.status().state,
            EquipmentState::Idle
        );
    }
}

TEST_F(WaferRoutesTests, RecoveryRejectsInvalidDestinationSlots)
{
    const std::vector<int> invalidSlots = {0, -1, -100};

    auto cli = client();

    for (const int slot : invalidSlots)
    {
        SCOPED_TRACE(slot);

        const auto response = cli.Post(
            "/api/wafers/recover",
            "{\"waferId\":\"W1\",\"destinationSlot\":" +
                std::to_string(slot) + "}",
            "application/json"
        );

        ASSERT_TRUE(response);
        EXPECT_EQ(response->status, 422);
    }
}


