
#include <gtest/gtest.h>

#include <atomic>
#include <thread>
#include <vector>

#include "controller/mini_fab_controller.h"

TEST(SchedulerLifecycleTest, ConcurrentStartsAreSafe)
{
    MiniFabController controller;

    constexpr int threadCount = 16;
    std::atomic<bool> go{false};

    std::vector<std::thread> threads;
    threads.reserve(threadCount);

    for (int i = 0; i < threadCount; ++i)
    {
        threads.emplace_back([&]()
        {
            while (!go.load(std::memory_order_acquire))
                std::this_thread::yield();

            controller.startScheduler();
        });
    }

    go.store(true, std::memory_order_release);

    for (auto& thread : threads)
        thread.join();

    EXPECT_EQ(
        controller.schedulerState(),
        SchedulerState::Running
    );

    controller.stopScheduler();

    EXPECT_EQ(
        controller.schedulerState(),
        SchedulerState::Stopped
    );
}

TEST(SchedulerLifecycleTest, ConcurrentStopsAreSafe)
{
    MiniFabController controller;
    controller.startScheduler();

    constexpr int threadCount = 16;
    std::atomic<bool> go{false};

    std::vector<std::thread> threads;
    threads.reserve(threadCount);

    for (int i = 0; i < threadCount; ++i)
    {
        threads.emplace_back([&]()
        {
            while (!go.load(std::memory_order_acquire))
                std::this_thread::yield();

            controller.stopScheduler();
        });
    }

    go.store(true, std::memory_order_release);

    for (auto& thread : threads)
        thread.join();

    EXPECT_EQ(
        controller.schedulerState(),
        SchedulerState::Stopped
    );
}

TEST(SchedulerLifecycleTest, ConcurrentStartStopDoesNotCorruptWorker)
{
    MiniFabController controller;

    constexpr int threadCount = 8;
    constexpr int iterations = 50;

    std::atomic<bool> go{false};

    std::vector<std::thread> threads;
    threads.reserve(threadCount);

    for (int i = 0; i < threadCount; ++i)
    {
        threads.emplace_back([&, i]()
        {
            while (!go.load(std::memory_order_acquire))
                std::this_thread::yield();

            for (int j = 0; j < iterations; ++j)
            {
                if ((i + j) % 2 == 0)
                    controller.startScheduler();
                else
                    controller.stopScheduler();
            }
        });
    }

    go.store(true, std::memory_order_release);

    for (auto& thread : threads)
        thread.join();

    // Concurrent ordering is nondeterministic.
    // Explicitly establish the final state.
    controller.stopScheduler();

    EXPECT_EQ(
        controller.schedulerState(),
        SchedulerState::Stopped
    );

    // Verify that a fresh worker can still start.
    controller.startScheduler();

    EXPECT_EQ(
        controller.schedulerState(),
        SchedulerState::Running
    );

    controller.stopScheduler();
}
