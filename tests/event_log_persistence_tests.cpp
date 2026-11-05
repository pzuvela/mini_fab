#include <gtest/gtest.h>

#include "equipment/event_log.h"
#include "persistence/sqlite_event_repository.h"

TEST(EventLogPersistenceTests, RestoresEventsAcrossInstances)
{
    SqliteEventRepository repository(":memory:");

    std::uint64_t firstId;

    {
        EventLog log({}, &repository);

        log.record(
            EventType::EquipmentStarted,
            "Equipment started"
        );

        auto events = log.getEvents();

        ASSERT_EQ(events.size(), 1);
        firstId = events[0].sequenceId;
    }

    {
        EventLog log({}, &repository);

        auto events = log.getEvents();

        ASSERT_EQ(events.size(), 1);
        EXPECT_EQ(events[0].sequenceId, firstId);
        EXPECT_EQ(events[0].message, "Equipment started");

        log.record(
            EventType::EquipmentIdle,
            "Equipment idle"
        );

        events = log.getEvents();

        ASSERT_EQ(events.size(), 2);
        EXPECT_GT(events[1].sequenceId, firstId);
    }
}

