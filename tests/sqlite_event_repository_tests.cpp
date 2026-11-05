#include <gtest/gtest.h>
#include <filesystem>

#include "persistence/sqlite_event_repository.h"

TEST(SqliteEventRepositoryTests, InsertAndRetrieve)
{
    SqliteEventRepository repo(":memory:");

    EquipmentEvent event{
        0,
        std::chrono::system_clock::time_point{
            std::chrono::milliseconds{1234567890000}
        },
        EventType::RunCompleted,
        EventSeverity::Info,
        "Wafer W1 completed"
    };

    auto id = repo.insert(event);
    auto events = repo.getAll();

    ASSERT_EQ(events.size(), 1);

    EXPECT_EQ(events[0].sequenceId, id);
    EXPECT_EQ(events[0].timestamp, event.timestamp);
    EXPECT_EQ(events[0].type, event.type);
    EXPECT_EQ(events[0].severity, event.severity);
    EXPECT_EQ(events[0].message, event.message);
}

TEST(SqliteEventRepositoryTests, PersistsAcrossConnections)
{
    namespace fs = std::filesystem;

    const auto path =
        fs::temp_directory_path() /
        "minifab_event_persistence_test.sqlite";

    fs::remove(path);

    try
    {
        std::uint64_t firstId;

        {
            SqliteEventRepository repo(path.string());

            firstId = repo.insert({
                0,
                std::chrono::system_clock::now(),
                EventType::RunFailed,
                EventSeverity::Error,
                "Wafer W1 failed"
            });
        }

        {
            SqliteEventRepository repo(path.string());

            auto events = repo.getAll();

            ASSERT_EQ(events.size(), 1);
            EXPECT_EQ(events[0].sequenceId, firstId);
            EXPECT_EQ(events[0].type, EventType::RunFailed);
            EXPECT_EQ(events[0].severity, EventSeverity::Error);
            EXPECT_EQ(events[0].message, "Wafer W1 failed");

            auto nextId = repo.insert({
                0,
                std::chrono::system_clock::now(),
                EventType::EquipmentStarted,
                EventSeverity::Info,
                "Equipment restarted"
            });

            EXPECT_GT(nextId, firstId);
        }
    }
    catch (...)
    {
        fs::remove(path);
        throw;
    }

    fs::remove(path);
}
