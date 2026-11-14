
#include <gtest/gtest.h>
#include <httplib.h>

#include <chrono>
#include <functional>
#include <string>
#include <thread>
#include <algorithm>
#include <array>
#include <latch>
#include <optional>
#include <charconv>
#include <cstdint>
#include <system_error>

#include "controller/mini_fab_controller.h"
#include "persistence/sqlite_recipe_repository.h"
#include "recipes/recipe_manager.h"
#include "web/routes/job_routes.h"
#include "web/routes/recipe_routes.h"
#include "web/routes/equipment_routes.h"
#include "web/routes/wafer_routes.h"


using namespace std::chrono_literals;

namespace
{
    bool waitUntil(
        const std::function<bool()>& condition,
        std::chrono::milliseconds timeout = 5s)
    {
        const auto deadline =
            std::chrono::steady_clock::now() + timeout;

        while (std::chrono::steady_clock::now() < deadline)
        {
            if (condition())
                return true;

            std::this_thread::sleep_for(10ms);
        }

        return condition();
    }
}

class JobRoutesTest : public ::testing::Test
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
        ASSERT_TRUE(
            controller_.addRecipe({1, "Fast Etch", 0.01})
        );

        MiniFab::Web::registerJobRoutes(
            server_,
            controller_
        );
        MiniFab::Web::registerRecipeRoutes(server_, controller_);
        MiniFab::Web::registerEquipmentRoutes(server_, controller_);
        MiniFab::Web::registerWaferRoutes(server_, controller_);

        // Ask the OS for an available localhost port.
        port_ = server_.bind_to_any_port("127.0.0.1");

        ASSERT_GT(port_, 0);

        serverThread_ = std::thread([this]()
        {
            server_.listen_after_bind();
        });

        ASSERT_TRUE(waitUntil([this]()
        {
            return server_.is_running();
        }));
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
        return httplib::Client(
            "127.0.0.1",
            port_
        );
    }

    httplib::Result postJob(
        const std::string& body =
            "waferId=W1&recipeId=1&processModuleId=1")
    {
        auto cli = client();

        return cli.Post(
            "/api/jobs",
            body,
            "application/x-www-form-urlencoded"
        );
    }
};

TEST_F(JobRoutesTest, CreatesJobSuccessfully)
{
    auto response = postJob();

    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 201);

    EXPECT_EQ(
        response->get_header_value("Location"),
        "/api/jobs/1"
    );

    EXPECT_EQ(
        response->body,
        R"({"jobId":1})"
    );

    EXPECT_EQ(controller_.pendingJobCount(), 1);
    EXPECT_EQ(controller_.jobStatus(1), JobStatus::Queued);
}

TEST_F(JobRoutesTest, RetrievesQueuedJob)
{
    ASSERT_TRUE(postJob());

    auto cli = client();
    auto response = cli.Get("/api/jobs/1");

    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 200);

    EXPECT_EQ(
        response->body,
        R"({"jobId":1,"status":"Queued"})"
    );
}

TEST_F(JobRoutesTest, RejectsMissingParameters)
{
    auto response = postJob(
        "waferId=W1&recipeId=1"
    );

    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 400);
    EXPECT_EQ(controller_.pendingJobCount(), 0);
}

TEST_F(JobRoutesTest, RejectsInvalidRecipeId)
{
    auto response = postJob(
        "waferId=W1&recipeId=abc&processModuleId=1"
    );

    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 400);
}

TEST_F(JobRoutesTest, RejectsNegativeRecipeId)
{
    auto response = postJob(
        "waferId=W1&recipeId=-1&processModuleId=1"
    );

    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 400);
}

TEST_F(JobRoutesTest, RejectsUnknownRecipe)
{
    auto response = postJob(
        "waferId=W1&recipeId=999&processModuleId=1"
    );

    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 404);
}

TEST_F(JobRoutesTest, RejectsUnsupportedProcessModule)
{
    auto response = postJob(
        "waferId=W1&recipeId=1&processModuleId=2"
    );

    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 422);
}

TEST_F(JobRoutesTest, RejectsUnknownWafer)
{
    auto response = postJob(
        "waferId=W999&recipeId=1&processModuleId=1"
    );

    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 404);
}

TEST_F(JobRoutesTest, Returns404ForUnknownJob)
{
    auto cli = client();
    auto response = cli.Get("/api/jobs/999");

    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 404);
}

TEST_F(JobRoutesTest, RejectsInvalidJobId)
{
    auto cli = client();
    auto response = cli.Get("/api/jobs/0");

    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 400);
}

TEST_F(JobRoutesTest, CreatesJobsWithUniqueIds)
{
ASSERT_EQ(
    controller_.registerWafer({
        "W2",
        {Station::LoadPort, 4}
    }),
    WaferRegistrationResult::Registered
);

auto first = postJob();
auto second = postJob(
    "waferId=W2&recipeId=1&processModuleId=1"
);

    ASSERT_TRUE(first);
    ASSERT_TRUE(second);

    EXPECT_EQ(first->status, 201);
    EXPECT_EQ(second->status, 201);

    EXPECT_EQ(first->body, R"({"jobId":1})");
    EXPECT_EQ(second->body, R"({"jobId":2})");

    EXPECT_EQ(controller_.pendingJobCount(), 2);
}

TEST_F(JobRoutesTest, JobCompletesThroughScheduler)
{
    auto response = postJob();

    ASSERT_TRUE(response);
    ASSERT_EQ(response->status, 201);

    controller_.startScheduler();

    ASSERT_TRUE(waitUntil([this]()
    {
        return controller_.jobStatus(1)
            == JobStatus::Completed;
    }));

    auto cli = client();
    auto statusResponse = cli.Get("/api/jobs/1");

    ASSERT_TRUE(statusResponse);
    EXPECT_EQ(statusResponse->status, 200);

    EXPECT_EQ(
        statusResponse->body,
        R"({"jobId":1,"status":"Completed"})"
    );

    EXPECT_EQ(controller_.pendingJobCount(), 0);
}

TEST_F(JobRoutesTest, ReturnsEmptyJobList)
{
    auto cli = client();
    auto response = cli.Get("/api/jobs");

    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 200);
    EXPECT_EQ(response->body, "[]");
}

TEST_F(JobRoutesTest, ListsQueuedJobsInOrder)
{
    ASSERT_EQ(
        controller_.registerWafer({
            "W2",
            {Station::LoadPort, 4}
        }),
        WaferRegistrationResult::Registered
    );

    // Enqueue two different wafers
    ASSERT_TRUE(postJob());
    ASSERT_TRUE(postJob(
        "waferId=W2&recipeId=1&processModuleId=1"
    ));

    // Retrieve queued jobs
    auto cli = client();
    auto response = cli.Get("/api/jobs");

    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 200);

    // Find positions of job IDs in JSON
    const auto first = response->body.find(R"("jobId":1)");
    const auto second = response->body.find(R"("jobId":2)");

    ASSERT_NE(first, std::string::npos);
    ASSERT_NE(second, std::string::npos);

    // Verify ordering
    EXPECT_LT(first, second);

    EXPECT_NE(
        response->body.find(R"("status":"Queued")"),
        std::string::npos
    );
}

TEST_F(JobRoutesTest, RetainsCompletedJobInList)
{
    ASSERT_TRUE(postJob());

    controller_.startScheduler();

    ASSERT_TRUE(waitUntil([this]()
    {
        return controller_.jobStatus(1)
            == JobStatus::Completed;
    }));

    auto cli = client();
    auto response = cli.Get("/api/jobs");

    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 200);

    EXPECT_NE(
        response->body.find(R"("jobId":1)"),
        std::string::npos
    );

    EXPECT_NE(
        response->body.find(R"("status":"Completed")"),
        std::string::npos
    );

    EXPECT_NE(
        response->body.find(R"("waferId":"W1")"),
        std::string::npos
    );
}


TEST_F(JobRoutesTest, RecipeDropdownReceivesSavedRecipes)
{
    ASSERT_TRUE(controller_.addRecipe({2, "Slow Etch", 0.02}));
    auto cli = client();
    auto response = cli.Get("/api/recipes");

    ASSERT_TRUE(response);
    ASSERT_EQ(response->status, 200);
    EXPECT_NE(response->body.find(R"("id":1)"), std::string::npos);
    EXPECT_NE(response->body.find(R"("name":"Fast Etch")"), std::string::npos);
    EXPECT_NE(response->body.find(R"("id":2)"), std::string::npos);
    EXPECT_NE(response->body.find(R"("name":"Slow Etch")"), std::string::npos);
}

TEST_F(JobRoutesTest, EnqueuesSelectedRecipeId)
{
    ASSERT_TRUE(controller_.addRecipe({2, "Slow Etch", 0.02}));
    auto response = postJob("waferId=W1&recipeId=2&processModuleId=1");
    ASSERT_TRUE(response);
    ASSERT_EQ(response->status, 201);

    auto cli = client();
    auto jobs = cli.Get("/api/jobs");
    ASSERT_TRUE(jobs);
    EXPECT_NE(jobs->body.find(R"("recipeId":2)"), std::string::npos);
    EXPECT_NE(jobs->body.find(R"("waferId":"W1")"), std::string::npos);
}

TEST_F(JobRoutesTest, EventLogReturnsEventsForJobsPage)
{
    auto cli = client();
    auto response = cli.Get("/api/events");
    ASSERT_TRUE(response);
    ASSERT_EQ(response->status, 200);
    EXPECT_NE(response->body.find(R"("events":[)"), std::string::npos);
    EXPECT_NE(response->body.find(R"("sequenceId":)"), std::string::npos);
    EXPECT_NE(response->body.find(R"("timestamp":)"), std::string::npos);
    EXPECT_NE(response->body.find(R"("severity":)"), std::string::npos);
    EXPECT_NE(response->body.find(R"("message":)"), std::string::npos);
}

TEST_F(JobRoutesTest, EventLogRecordsCompletedScheduledWafer)
{
    ASSERT_EQ(postJob()->status, 201);
    controller_.startScheduler();
    ASSERT_TRUE(waitUntil([this] {
        return controller_.jobStatus(1) == JobStatus::Completed;
    }));

    auto cli = client();
    auto response = cli.Get("/api/events");
    ASSERT_TRUE(response);
    ASSERT_EQ(response->status, 200);
    EXPECT_NE(response->body.find("Processing completed for wafer W1"), std::string::npos);
}

TEST_F(JobRoutesTest, RejectsEmptyWaferIdFromEditableField)
{
    auto response = postJob("waferId=&recipeId=1&processModuleId=1");
    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 400);
    EXPECT_EQ(controller_.pendingJobCount(), 0);
}

TEST_F(JobRoutesTest, AcceptsRegisteredSecondWafer)
{
    const Wafer wafer{
        "W2",
        {Station::LoadPort, 4}
    };

    ASSERT_EQ(
        controller_.registerWafer(wafer),
        WaferRegistrationResult::Registered
    );

    // Use an existing valid recipe ID in your fixture.
    auto cli = client();

    const auto response = cli.Post(
        "/api/jobs",
        "waferId=W2&recipeId=1&processModuleId=1",
        "application/x-www-form-urlencoded"
    );

    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 201);
}

TEST_F(JobRoutesTest, RejectsDuplicateQueuedWafer)
{
    // First submission succeeds
    auto first = postJob();

    ASSERT_TRUE(first);
    ASSERT_EQ(first->status, 201);
    EXPECT_EQ(first->body, R"({"jobId":1})");

    // Same wafer, second submission
    auto duplicate = postJob();

    ASSERT_TRUE(duplicate);

    // A queued wafer cannot be queued again
    EXPECT_EQ(duplicate->status, 409);
    EXPECT_EQ(
        duplicate->body,
        R"({"error":"Wafer already queued"})"
    );

    // No second job was created
    EXPECT_EQ(controller_.pendingJobCount(), 1u);

    const auto jobs = controller_.jobsSnapshot();

    ASSERT_EQ(jobs.size(), 1u);
    EXPECT_EQ(jobs.front().job.waferId, "W1");
    EXPECT_EQ(jobs.front().status, JobStatus::Queued);
}

TEST_F(JobRoutesTest, RunEndpointAcceptsSelectedWafer)
{
    // Register a second wafer.
    ASSERT_EQ(
        controller_.registerWafer({
            "W2",
            {Station::LoadPort, 4}
        }),
        WaferRegistrationResult::Registered
    );

    auto cli = client();

    // Explicitly request W2, not the default W1.
    auto response = cli.Post(
        "/api/run",
        "waferId=W2&recipeId=1",
        "application/x-www-form-urlencoded"
    );

    ASSERT_TRUE(response);
    ASSERT_EQ(response->status, 202);

    // The equipment should have started W2.
    const auto snapshot = controller_.status();

    EXPECT_TRUE(
        snapshot.robotWaferId == "W2" ||
        snapshot.processModuleWaferId == "W2"
    );
}

TEST_F(JobRoutesTest, RunRejectsMissingWaferId)
{
    auto response = client().Post(
        "/api/run",
        "recipeId=1",
        "application/x-www-form-urlencoded"
    );

    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 400);
    EXPECT_EQ(
        response->body,
        R"({"error":"Missing wafer ID"})"
    );
}

TEST_F(JobRoutesTest, RunRejectsUnknownWafer)
{
    auto response = client().Post(
        "/api/run",
        "waferId=UNKNOWN&recipeId=1",
        "application/x-www-form-urlencoded"
    );

    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 404);
    EXPECT_EQ(
        response->body,
        R"({"error":"Wafer not found"})"
    );
}



namespace
{
    struct ConcurrentJobResponses
    {
        std::array<int, 2> statuses{};
        std::array<std::string, 2> bodies;
    };

    ConcurrentJobResponses submitConcurrentDuplicateJobs(int port)
    {
        ConcurrentJobResponses result;
        std::latch startGate(1);

        auto submit = [&](std::size_t index)
        {
            httplib::Client cli("127.0.0.1", port);
            startGate.wait();

            const auto response = cli.Post(
                "/api/jobs",
                "waferId=W1&recipeId=1&processModuleId=1",
                "application/x-www-form-urlencoded"
            );

            if (response)
            {
                result.statuses[index] = response->status;
                result.bodies[index] = response->body;
            }
        };

        std::thread first(submit, 0);
        std::thread second(submit, 1);
        startGate.count_down();
        first.join();
        second.join();
        std::sort(result.statuses.begin(), result.statuses.end());
        return result;
    }

    bool registerW2ViaHttp(httplib::Client& cli)
    {
        const auto response = cli.Post(
            "/api/wafers",
            R"({"id":"W2","station":"LoadPort","slot":4})",
            "application/json"
        );
        return response && response->status == 201;
    }

    std::optional<std::uint64_t> startW2ViaHttp(httplib::Client& cli)
    {
        const auto response = cli.Post(
            "/api/run",
            "waferId=W2&recipeId=1",
            "application/x-www-form-urlencoded"
        );

        if (!response || response->status != 202 ||
            response->body.find(R"("accepted":true)") == std::string::npos)
            return std::nullopt;

        const auto position = response->body.find(R"("runId":)");
        if (position == std::string::npos)
            return std::nullopt;

        const auto value = response->body.substr(position + 8);
        std::uint64_t runId = 0;
        const auto [end, error] = std::from_chars(
            value.data(), value.data() + value.size(), runId
        );
        if (error != std::errc{} || end == value.data() || runId == 0)
            return std::nullopt;

        return runId;
    }

    bool hasCompletedRun(const MiniFabController& controller, std::uint64_t runId)
    {
        const auto events = controller.events();
        return std::any_of(events.begin(), events.end(),
            [runId](const EquipmentEvent& event)
            {
                return event.runId == runId &&
                    event.type == EventType::RunCompleted;
            });
    }
    bool runW2ToCompletion(httplib::Client& cli, const MiniFabController& controller)
    {
        if (!registerW2ViaHttp(cli))
            return false;
        const auto runId = startW2ViaHttp(cli);
        return runId.has_value() && waitUntil([&]()
        {
            return hasCompletedRun(controller, *runId);
        });
    }

}

TEST_F(JobRoutesTest, ConcurrentDuplicateRequestsReturnCreatedAndConflict)
{
    const auto result = submitConcurrentDuplicateJobs(port_);

    EXPECT_EQ(result.statuses[0], 201);
    EXPECT_EQ(result.statuses[1], 409);
    EXPECT_EQ(controller_.pendingJobCount(), 1u);
}

TEST_F(JobRoutesTest, ConcurrentDuplicateRequestsPreserveSingleJobIdentity)
{
    const auto result = submitConcurrentDuplicateJobs(port_);
    const auto jobs = controller_.jobsSnapshot();

    ASSERT_EQ(jobs.size(), 1u);
    EXPECT_EQ(jobs.front().job.waferId, "W1");
    EXPECT_EQ(
        std::count_if(result.bodies.begin(), result.bodies.end(),
            [](const std::string& body)
            {
                return body.find(R"("jobId":)") != std::string::npos;
            }),
        1
    );
}

TEST_F(JobRoutesTest, RegisteredW2AppearsInHttpInventory)
{
    auto cli = client();
    ASSERT_TRUE(registerW2ViaHttp(cli));

    const auto response = cli.Get("/api/wafers");
    ASSERT_TRUE(response);
    EXPECT_NE(response->body.find(R"("id":"W2")"), std::string::npos);
}

TEST_F(JobRoutesTest, SelectedW2RunProducesCorrelatedCompletion)
{
    auto cli = client();
    ASSERT_TRUE(registerW2ViaHttp(cli));

    const auto runId = startW2ViaHttp(cli);
    ASSERT_TRUE(runId.has_value());
    EXPECT_TRUE(waitUntil([&]()
    {
        return hasCompletedRun(controller_, *runId);
    }));
}

TEST_F(JobRoutesTest, CompletedW2RunLeavesEquipmentIdleAndEmpty)
{
    auto cli = client();
    ASSERT_TRUE(runW2ToCompletion(cli, controller_));

    const auto equipment = controller_.status();
    EXPECT_EQ(equipment.state, EquipmentState::Idle);
    EXPECT_FALSE(equipment.robotWaferId.has_value());
    EXPECT_FALSE(equipment.processModuleWaferId.has_value());
}

TEST_F(JobRoutesTest, CompletedW2RunPreservesBothWaferLocations)
{
    auto cli = client();
    ASSERT_TRUE(runW2ToCompletion(cli, controller_));

    const auto wafers = controller_.wafersSnapshot();
    const auto findWafer = [&](const std::string& id)
    {
        return std::find_if(wafers.begin(), wafers.end(),
            [&](const Wafer& wafer) { return wafer.id == id; });
    };
    const auto w1 = findWafer("W1");
    const auto w2 = findWafer("W2");

    ASSERT_TRUE(w1 != wafers.end() && w2 != wafers.end());
    EXPECT_EQ(w1->location, (Location{Station::LoadPort, 3}));
    EXPECT_EQ(w2->location, (Location{Station::LoadPort, 4}));
}
