
#include <gtest/gtest.h>

#include <chrono>
#include <vector>

#include "web/serialization/equipment_events_json.h"

TEST(EquipmentEventsJsonTest, SerializesEmptyEvents)
{
    const std::vector<EquipmentEvent> events;

    EXPECT_EQ(
        MiniFab::Web::toJson(events),
        R"({"events":[]})"
    );
}

TEST(EquipmentEventsJsonTest, SerializesSingleEvent)
{
    const EquipmentEvent event{
        1,
        std::chrono::system_clock::time_point{
            std::chrono::milliseconds{123456}
        },
        EventType::RunRequested,
        EventSeverity::Info,
        "Run requested",
        42
    };

    EXPECT_EQ(
        MiniFab::Web::toJson({event}),
        R"({"events":[{"sequenceId":1,"timestamp":123456,"type":"RunRequested","severity":"Info","message":"Run requested","runId":42}]})"
    );
}

TEST(EquipmentEventsJsonTest, EscapesEventMessage)
{
    const EquipmentEvent event{
        2,
        std::chrono::system_clock::time_point{},
        EventType::SystemError,
        EventSeverity::Error,
        "Robot \"R1\"\nFailed",
        7
    };

    EXPECT_EQ(
        MiniFab::Web::toJson({event}),
        R"({"events":[{"sequenceId":2,"timestamp":0,"type":"SystemError","severity":"Error","message":"Robot \"R1\"\nFailed","runId":7}]})"
    );
}

TEST(EquipmentEventsJsonTest, PreservesEventOrder)
{
    const auto timestamp =
        std::chrono::system_clock::time_point{};

    const std::vector<EquipmentEvent> events{
        {10, timestamp, EventType::RunRequested,
         EventSeverity::Info, "First", 1},

        {11, timestamp, EventType::RunCompleted,
         EventSeverity::Info, "Second", 1}
    };

    EXPECT_EQ(
        MiniFab::Web::toJson(events),
        R"({"events":[{"sequenceId":10,"timestamp":0,"type":"RunRequested","severity":"Info","message":"First","runId":1},{"sequenceId":11,"timestamp":0,"type":"RunCompleted","severity":"Info","message":"Second","runId":1}]})"
    );
}

