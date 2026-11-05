
#include <gtest/gtest.h>
#include <httplib.h>

#include <chrono>
#include <functional>
#include <string>
#include <thread>

#include "controller/mini_fab_controller.h"
#include "persistence/sqlite_recipe_repository.h"
#include "recipes/recipe_manager.h"
#include "web/routes/job_routes.h"
#include "web/routes/recipe_routes.h"
#include "web/routes/equipment_routes.h"

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
    EXPECT_EQ(response->status, 422);
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
    auto first = postJob();
    auto second = postJob();

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
    ASSERT_TRUE(postJob());
    ASSERT_TRUE(postJob());

    auto cli = client();
    auto response = cli.Get("/api/jobs");

    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 200);

    const auto first = response->body.find(R"("jobId":1)");
    const auto second = response->body.find(R"("jobId":2)");

    ASSERT_NE(first, std::string::npos);
    ASSERT_NE(second, std::string::npos);

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
