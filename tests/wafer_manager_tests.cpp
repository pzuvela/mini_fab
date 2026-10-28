#include <gtest/gtest.h>

#include "wafer_manager.h"
#include "domain/wafer.h"
#include "domain/location.h"

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
		EXPECT_EQ(*wafer, wafer_);
	}

	TEST_F(WaferManagerTest, ReturnsNullWhenWaferDoesNotExist) 
	{
		WaferManager mgr;
		Wafer* wafer = mgr.findWafer("2");
		EXPECT_EQ(wafer, nullptr);
	}

}


