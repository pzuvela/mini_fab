#include <gtest/gtest.h>

#include "wafer/wafer_manager.h"
#include "wafer/location_update_result.h"
#include "domain/wafer.h"
#include "domain/location.h"
#include "mocks/mock_robot.h"
#include "mocks/mock_wafer_manager.h"

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
			.slot = 3
		};

	};

	TEST_F(WaferManagerTest, AddsWaferWhenIdDoesNotExist)
	{
		WaferManager mgr;
                bool isWaferAdded = mgr.addWafer(wafer_);
		EXPECT_TRUE(isWaferAdded);
	}

	TEST_F(WaferManagerTest, DoesNotAddWaferWhenIdAlreadyExists)
	{
		WaferManager mgr;
		mgr.addWafer(wafer_);
		bool isWaferAdded = mgr.addWafer(wafer_);
		EXPECT_FALSE(isWaferAdded);
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
}
