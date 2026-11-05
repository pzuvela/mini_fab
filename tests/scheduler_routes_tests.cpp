#include <gtest/gtest.h>
#include <httplib.h>

#include <chrono>
#include <thread>

#include "controller/mini_fab_controller.h"
#include "web/routes/scheduler_routes.h"

using namespace std::chrono_literals;

class SchedulerRoutesTest : public ::testing::Test
{
protected:
    MiniFabController controller_;
    httplib::Server server_;
    std::thread worker_;
    int port_ = -1;

    void SetUp() override
    {
        MiniFab::Web::registerSchedulerRoutes(
            server_, controller_
        );

        port_ = server_.bind_to_any_port("127.0.0.1");
        ASSERT_GT(port_, 0);

        worker_ = std::thread([this]()
        {
            server_.listen_after_bind();
        });

        const auto deadline =
            std::chrono::steady_clock::now() + 5s;

        while (!server_.is_running() &&
               std::chrono::steady_clock::now() < deadline)
        {
            std::this_thread::sleep_for(10ms);
        }

        ASSERT_TRUE(server_.is_running());
    }

    void TearDown() override
    {
        server_.stop();

        if (worker_.joinable())
            worker_.join();

        controller_.stopScheduler();
    }

    httplib::Client client() const
    {
        return httplib::Client("127.0.0.1", port_);
    }
};

TEST_F(SchedulerRoutesTest, InitiallyStopped)
{
    auto cli = client();
    auto response = cli.Get("/api/scheduler/status");

    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 200);
    EXPECT_NE(response->body.find(R"("state":"Stopped")"),
              std::string::npos);
    EXPECT_NE(response->body.find(R"("pendingJobs":0)"),
              std::string::npos);
}

TEST_F(SchedulerRoutesTest, StartsScheduler)
{
    auto cli = client();
    auto response = cli.Post("/api/scheduler/start");

    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 200);
    EXPECT_EQ(controller_.schedulerState(), SchedulerState::Running);
}

TEST_F(SchedulerRoutesTest, StartIsIdempotent)
{
    auto cli = client();

    ASSERT_TRUE(cli.Post("/api/scheduler/start"));
    auto response = cli.Post("/api/scheduler/start");

    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 200);
    EXPECT_EQ(controller_.schedulerState(), SchedulerState::Running);
}

TEST_F(SchedulerRoutesTest, StopsScheduler)
{
    auto cli = client();

    ASSERT_TRUE(cli.Post("/api/scheduler/start"));
    auto response = cli.Post("/api/scheduler/stop");

    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 200);
    EXPECT_EQ(controller_.schedulerState(), SchedulerState::Stopped);
}

TEST_F(SchedulerRoutesTest, StopIsIdempotent)
{
    auto cli = client();

    auto first = cli.Post("/api/scheduler/stop");
    auto second = cli.Post("/api/scheduler/stop");

    ASSERT_TRUE(first);
    ASSERT_TRUE(second);

    EXPECT_EQ(first->status, 200);
    EXPECT_EQ(second->status, 200);
}

TEST_F(SchedulerRoutesTest, CannotResetHealthyScheduler)
{
    auto cli = client();
    auto response = cli.Post("/api/scheduler/reset");

    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 409);
}


