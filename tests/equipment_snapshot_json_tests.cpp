#include <gtest/gtest.h>
#include "web/serialization/equipment_snapshot_json.h"

TEST(EquipmentSnapshotJsonTest, SerializesIdleState)
{
    EquipmentSnapshot snapshot{
        .state = EquipmentState::Idle,
        .waferLocation = Location{Station::LoadPort, 3},
        .robotWaferId = std::nullopt,
        .processModuleWaferId = std::nullopt
    };

    std::string result = MiniFab::Web::toJson(snapshot);

    EXPECT_EQ(
        result,
        R"({"equipmentState":"Idle","waferLocation":{"station":"LoadPort","slot":3},"robotWaferId":null,"processModuleWaferId":null})"
    );
}

