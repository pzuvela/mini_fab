#include <gtest/gtest.h>
#include <stdexcept>

#include "sim_process_module.h"
#include "domain/wafer.h"
#include "domain/recipe.h"
#include "domain/location.h"

namespace MiniFab::Tests
{
    // Arrange, Act, Assert -> AAA principle for testing

    class SimProcessModuleTest : public ::testing::Test
    {
    protected:
        Wafer wafer{
            .id = "1",
            .location = Location{
                .station = Station::LoadPort,
                .slot = 2
            }
        };

        Recipe recipe{
            .id = 1,
            .name = "Recipe1",
            .durationSeconds = 55.2
        };
    };

    TEST_F(SimProcessModuleTest, IsNotReadyBeforeInitialization)
    {
        SimProcessModule module = SimProcessModule();

        bool isReady = module.isReady();

        EXPECT_FALSE(isReady);
    }

    TEST_F(SimProcessModuleTest, IsReadyAfterInitialization)
    {
        SimProcessModule module = SimProcessModule();

        module.initialize();

        bool isReady = module.isReady();

        EXPECT_TRUE(isReady);
    }


    TEST_F(SimProcessModuleTest, ProcessSucceedsWhenInitialized)
    {
        SimProcessModule module = SimProcessModule();

        module.initialize();

        bool isSuccessful = module.process(wafer, recipe);

        EXPECT_TRUE(isSuccessful);
    }

    TEST_F(SimProcessModuleTest, ProcessThrowsWhenNotInitialized)
    {
        SimProcessModule module = SimProcessModule();

        EXPECT_THROW(module.process(wafer, recipe), std::logic_error);
    }
}