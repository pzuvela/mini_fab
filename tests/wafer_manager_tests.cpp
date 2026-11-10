#include <gtest/gtest.h>

#include <algorithm>
#include <stdexcept>
#include <thread>

#include "wafer/wafer_manager.h"
#include "wafer/location_update_result.h"
#include "domain/wafer.h"
#include "domain/location.h"
#include "mocks/mock_robot.h"
#include "mocks/mock_wafer_manager.h"
#include "wafer/wafer_registration_result.h"
#include "persistence/sqlite_wafer_repository.h"


namespace MiniFab::Tests
{
	class WaferManagerTest : public ::testing::Test
	{
	protected:
		Wafer wafer_{
			.id = "1",
			.location = Location{
				.station = Station::LoadPort,
				.slot = 2
			}
		};

		Location location_{
			.station = Station::Aligner,
			.slot = 1
		};

	};

	TEST_F(WaferManagerTest, AddsWaferWhenIdDoesNotExist)
	{
		WaferManager mgr;
                WaferRegistrationResult result= mgr.addWafer(wafer_);
		EXPECT_EQ(result, WaferRegistrationResult::Registered);
	}

	TEST_F(WaferManagerTest, DoesNotAddWaferWhenIdAlreadyExists)
	{
		WaferManager mgr;
		mgr.addWafer(wafer_);
		WaferRegistrationResult result = mgr.addWafer(wafer_);
		EXPECT_EQ(result, WaferRegistrationResult::DuplicateId);
	}

	TEST_F(WaferManagerTest, FindsWaferById)
	{
		WaferManager mgr;
		mgr.addWafer(wafer_);
		Wafer* wafer = mgr.findWafer(wafer_.id);
		ASSERT_NE(wafer, nullptr);
		EXPECT_EQ(*wafer, wafer_);
	}

	TEST_F(WaferManagerTest, ReturnsNullWhenWaferDoesNotExist) 
	{
		WaferManager mgr;
		Wafer* wafer = mgr.findWafer("2");
		EXPECT_EQ(wafer, nullptr);
	}

	TEST_F(WaferManagerTest, ReturnsWaferNotFoundWhenWaferDoesNotExist)
	{
		WaferManager mgr;

		mgr.addWafer(wafer_);

		Wafer* managedWafer = mgr.findWafer(wafer_.id);

		Location originalLocation = managedWafer->location;

		LocationUpdateResult result = mgr.updateLocation("Wafer", location_);

		EXPECT_EQ(result, LocationUpdateResult::WaferNotFound);
		EXPECT_EQ(managedWafer->location, originalLocation);
	}

	TEST_F(WaferManagerTest, UpdatesLocationWhenWaferExists)
	{
		WaferManager mgr;

		mgr.addWafer(wafer_);

		LocationUpdateResult result = mgr.updateLocation(wafer_.id, location_);

		Wafer* managedWafer = mgr.findWafer(wafer_.id);

		EXPECT_EQ(result, LocationUpdateResult::Updated);
		EXPECT_NE(wafer_.location, location_);
		EXPECT_EQ(managedWafer->location, location_);
	}

	TEST_F(WaferManagerTest, ReturnsLocationSnapshot)
	{
	    WaferManager mgr;

	    mgr.addWafer({
	        "W1",
	        Location{Station::LoadPort, 3}
	    });

	    auto location = mgr.getLocation("W1");

	    ASSERT_TRUE(location.has_value());
	    EXPECT_EQ(location->station, Station::LoadPort);
	    EXPECT_EQ(location->slot, 3);
	}

	TEST_F(WaferManagerTest, UnknownWaferReturnsNullOpt)
	{
	    WaferManager mgr;

	    auto location = mgr.getLocation("Bla");

	    ASSERT_EQ(location, std::nullopt);
	}

TEST_F(WaferManagerTest, RejectsOccupiedLocation)
{
    WaferManager manager;

    ASSERT_EQ(manager.addWafer({"W1", {Station::LoadPort, 3}}), WaferRegistrationResult::Registered);
    EXPECT_EQ(manager.addWafer({"W2", {Station::LoadPort, 3}}), WaferRegistrationResult::SlotOccupied);
}

TEST_F(WaferManagerTest, GetWafersReturnsAllRegisteredWafers)
{
    WaferManager manager;
    ASSERT_EQ(manager.addWafer({"W1", {Station::LoadPort, 1}}), WaferRegistrationResult::Registered);
    ASSERT_EQ(manager.addWafer({"W2", {Station::LoadPort, 2}}), WaferRegistrationResult::Registered);

    const auto wafers = manager.getWafers();

    ASSERT_EQ(wafers.size(), 2);
    EXPECT_TRUE(std::ranges::any_of(wafers, [](const Wafer& wafer) {
        return wafer == Wafer{"W1", {Station::LoadPort, 1}};
    }));
    EXPECT_TRUE(std::ranges::any_of(wafers, [](const Wafer& wafer) {
        return wafer == Wafer{"W2", {Station::LoadPort, 2}};
    }));
}

TEST_F(WaferManagerTest, ModifyingSnapshotDoesNotModifyInventory)
{
    WaferManager manager;
    ASSERT_EQ(manager.addWafer({"W1", {Station::LoadPort, 1}}), WaferRegistrationResult::Registered);

    auto wafers = manager.getWafers();
    ASSERT_EQ(wafers.size(), 1);
    wafers.front().location = {Station::LoadPort, 10};

    EXPECT_EQ(manager.getLocation("W1"), (Location{Station::LoadPort, 1}));
}


TEST_F(WaferManagerTest, RejectsInvalidDestinationSlot)
{
    WaferManager manager;

    ASSERT_EQ(
        manager.addWafer({"W1", {Station::LoadPort, 1}}),
        WaferRegistrationResult::Registered
    );

    EXPECT_EQ(
        manager.updateLocation("W1", {Station::ProcessModule, 0}),
        LocationUpdateResult::InvalidLocation
    );

    EXPECT_EQ(
        manager.getLocation("W1"),
        (Location{Station::LoadPort, 1})
    );
}

TEST_F(WaferManagerTest, RejectsInvalidDestinationStation)
{
    WaferManager manager;

    ASSERT_EQ(
        manager.addWafer({"W1", {Station::LoadPort, 1}}),
        WaferRegistrationResult::Registered
    );

    EXPECT_EQ(
        manager.updateLocation(
            "W1",
            {static_cast<Station>(999), 1}
        ),
        LocationUpdateResult::InvalidLocation
    );

    EXPECT_EQ(
        manager.getLocation("W1"),
        (Location{Station::LoadPort, 1})
    );
}

TEST_F(WaferManagerTest, RejectsOccupiedDestination)
{
    WaferManager manager;

    ASSERT_EQ(
        manager.addWafer({"W1", {Station::LoadPort, 1}}),
        WaferRegistrationResult::Registered
    );

    ASSERT_EQ(
        manager.addWafer({"W2", {Station::LoadPort, 2}}),
        WaferRegistrationResult::Registered
    );

    EXPECT_EQ(
        manager.updateLocation("W2", {Station::LoadPort, 1}),
        LocationUpdateResult::SlotOccupied
    );

    EXPECT_EQ(
        manager.getLocation("W1"),
        (Location{Station::LoadPort, 1})
    );

    EXPECT_EQ(
        manager.getLocation("W2"),
        (Location{Station::LoadPort, 2})
    );
}

TEST_F(WaferManagerTest, UpdatingToSameLocationSucceeds)
{
    WaferManager manager;

    ASSERT_EQ(
        manager.addWafer({"W1", {Station::LoadPort, 1}}),
        WaferRegistrationResult::Registered
    );

    EXPECT_EQ(
        manager.updateLocation("W1", {Station::LoadPort, 1}),
        LocationUpdateResult::Updated
    );

    EXPECT_EQ(
        manager.getLocation("W1"),
        (Location{Station::LoadPort, 1})
    );
}

TEST_F(WaferManagerTest, LocationUpdatePersistsAcrossManagerRestart)
{
    SqliteWaferRepository repository(":memory:");

    {
        WaferManager manager(repository);

        ASSERT_EQ(
            manager.addWafer({"W1", {Station::LoadPort, 1}}),
            WaferRegistrationResult::Registered
        );

        ASSERT_EQ(
            manager.updateLocation(
                "W1",
                {Station::ProcessModule, 1}
            ),
            LocationUpdateResult::Updated
        );
    }

    // Reconstruct manager using the same repository.
    {
        WaferManager restoredManager(repository);

        const auto location = restoredManager.getLocation("W1");

        ASSERT_TRUE(location.has_value());

        EXPECT_EQ(
            *location,
            (Location{Station::ProcessModule, 1})
        );

        EXPECT_EQ(restoredManager.getWafers().size(), 1);
    }
}


TEST_F(WaferManagerTest, RejectedMovementDoesNotModifyDatabase)
{
    SqliteWaferRepository repository(":memory:");

    WaferManager manager(repository);

    ASSERT_EQ(
        manager.addWafer({"W1", {Station::LoadPort, 1}}),
        WaferRegistrationResult::Registered
    );

    ASSERT_EQ(
        manager.addWafer({"W2", {Station::LoadPort, 2}}),
        WaferRegistrationResult::Registered
    );

    EXPECT_EQ(
        manager.updateLocation("W2", {Station::LoadPort, 1}),
        LocationUpdateResult::SlotOccupied
    );

    EXPECT_EQ(
        manager.getLocation("W2"),
        (Location{Station::LoadPort, 2})
    );

    const auto persisted = repository.getAll();

    const auto it = std::find_if(
        persisted.begin(),
        persisted.end(),
        [](const Wafer& wafer)
        {
            return wafer.id == "W2";
        }
    );

    ASSERT_NE(it, persisted.end());

    EXPECT_EQ(
        it->location,
        (Location{Station::LoadPort, 2})
    );
}

TEST_F(WaferManagerTest, RejectsInvalidProcessModuleSlot)
{
    WaferManager manager;

    ASSERT_EQ(
        manager.addWafer({"W1", {Station::LoadPort, 1}}),
        WaferRegistrationResult::Registered
    );

    EXPECT_EQ(
        manager.updateLocation(
            "W1",
            {Station::ProcessModule, 2}
        ),
        LocationUpdateResult::InvalidLocation
    );

    EXPECT_EQ(
        manager.getLocation("W1"),
        (Location{Station::LoadPort, 1})
    );
}

TEST_F(WaferManagerTest, ConcurrentMovesCannotOccupySameLocation)
{
    WaferManager manager;

    ASSERT_EQ(
        manager.addWafer({"W1", {Station::LoadPort, 1}}),
        WaferRegistrationResult::Registered
    );

    ASSERT_EQ(
        manager.addWafer({"W2", {Station::LoadPort, 2}}),
        WaferRegistrationResult::Registered
    );

    LocationUpdateResult result1{};
    LocationUpdateResult result2{};

    const Location destination{Station::ProcessModule, 1};

    std::thread t1([&] {
        result1 = manager.updateLocation("W1", destination);
    });

    std::thread t2([&] {
        result2 = manager.updateLocation("W2", destination);
    });

    t1.join();
    t2.join();

    const bool firstWins =
        result1 == LocationUpdateResult::Updated &&
        result2 == LocationUpdateResult::SlotOccupied;

    const bool secondWins =
        result2 == LocationUpdateResult::Updated &&
        result1 == LocationUpdateResult::SlotOccupied;

    EXPECT_TRUE(firstWins || secondWins);

    const auto location1 = manager.getLocation("W1");
    const auto location2 = manager.getLocation("W2");

    ASSERT_TRUE(location1.has_value());
    ASSERT_TRUE(location2.has_value());

    EXPECT_NE(*location1, *location2);
}

TEST_F(WaferManagerTest, ReturnsWaferToOriginalLoadPortSlot)
{
    WaferManager manager;

    const Location original{Station::LoadPort, 3};
    const Location processModule{Station::ProcessModule, 1};

    ASSERT_EQ(
        manager.addWafer({"W1", original}),
        WaferRegistrationResult::Registered
    );

    ASSERT_EQ(
        manager.updateLocation("W1", processModule),
        LocationUpdateResult::Updated
    );

    EXPECT_EQ(
        manager.updateLocation("W1", original),
        LocationUpdateResult::Updated
    );

    EXPECT_EQ(manager.getLocation("W1"), original);
}

TEST_F(WaferManagerTest, LocationAvailabilityRespectsOccupancy)
{
    WaferManager manager;

    ASSERT_EQ(
        manager.addWafer({"W1", {Station::LoadPort, 1}}),
        WaferRegistrationResult::Registered
    );

    EXPECT_TRUE(manager.isLocationAvailable(
        "W2", {Station::ProcessModule, 1}
    ));

    ASSERT_EQ(
        manager.updateLocation(
            "W1", {Station::ProcessModule, 1}
        ),
        LocationUpdateResult::Updated
    );

    EXPECT_FALSE(manager.isLocationAvailable(
        "W2", {Station::ProcessModule, 1}
    ));

    EXPECT_TRUE(manager.isLocationAvailable(
        "W1", {Station::ProcessModule, 1}
    ));

    EXPECT_FALSE(manager.isLocationAvailable(
        "W2", {Station::ProcessModule, 2}
    ));
}

TEST_F(ProcessEngineTest, RejectsRunWhenMockReportsDestinationOccupied)
{
    auto wafer = std::make_unique<Wafer>(wafer_);

    MockWaferManager manager(std::move(wafer));
    manager.setLocationAvailable(false);

    MockRobot robot{true};
    MockProcessModule processModule{true};

    ProcessEngine engine(
        manager, robot, processModule, eventLog_
    );

    EXPECT_EQ(
        engine.startProcessWafer(
            wafer_.id,
            destination_,
            recipe_
        ),
        StartResult::Rejected
    );

    EXPECT_EQ(engine.state(), EquipmentState::Idle);
    EXPECT_FALSE(robot.moveWasCalled());
}



}


