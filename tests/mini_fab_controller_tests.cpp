#include <gtest/gtest.h>

#include "controller/mini_fab_controller.h"

TEST(MiniFabControllerTest, StartsWithWaferAtLoadPort)
{
    MiniFabController controller;

    EquipmentSnapshot snapshot = controller.status();

    EXPECT_EQ(snapshot.state, EquipmentState::Idle);

    ASSERT_TRUE(snapshot.waferLocation.has_value());

    EXPECT_EQ(
        snapshot.waferLocation.value(),
        (Location{Station::LoadPort, 3})
    );
}

