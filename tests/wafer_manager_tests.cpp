#include <gtest/gtest.h>

#include <algorithm>

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

TEST_F(WaferManagerTest, RejectsOccupiedLocation)
{
    WaferManager manager;

    EXPECT_TRUE(manager.addWafer({"W1", {Station::LoadPort, 3}}));
    EXPECT_FALSE(manager.addWafer({"W2", {Station::LoadPort, 3}}));
}

TEST(WaferManagerTest, GetWafersReturnsIndependentSnapshot)
{
    WaferManager manager;

    ASSERT_TRUE(manager.addWafer({"W1", {Station::LoadPort, 1}}));
    ASSERT_TRUE(manager.addWafer({"W2", {Station::LoadPort, 2}}));

    auto wafers = manager.getWafers();

    ASSERT_EQ(wafers.size(), 2);

    // Unordered map: don't assume iteration order!
    auto findWafer = [&](const std::string& id) {
        return std::find_if(
            wafers.begin(),
            wafers.end(),
            [&](const Wafer& wafer) {
                return wafer.id == id;
            }
        );
    };

    auto w1 = findWafer("W1");
    auto w2 = findWafer("W2");

    ASSERT_NE(w1, wafers.end());
    ASSERT_NE(w2, wafers.end());

    EXPECT_EQ(w1->location, (Location{Station::LoadPort, 1}));
    EXPECT_EQ(w2->location, (Location{Station::LoadPort, 2}));

    // Modify the snapshot
    w1->location = {Station::LoadPort, 10};

    // Original inventory must remain unchanged
    auto originalLocation = manager.getLocation("W1");

    ASSERT_TRUE(originalLocation.has_value());
    EXPECT_EQ(
        originalLocation.value(),
        (Location{Station::LoadPort, 1})
    );
}



}


