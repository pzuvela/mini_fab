#include <gtest/gtest.h>
#include <stdexcept>
#include <thread>

#include "equipment/operation_result.h"
#include "equipment/process_module/sim_process_module.h"
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

        Wafer wafer2{
            .id = "2",
            .location = Location{
                .station = Station::LoadPort,
                .slot = 4
            }
        };

        Recipe recipe{
            .id = 1,
            .name = "Recipe1",
            .durationSeconds = 0.05
        };

        Recipe longerRecipe{
            .id = 1,
            .name = "LongerRecipe2",
            .durationSeconds = 5
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

        OperationResult operationResult = module.process(wafer, recipe, std::stop_token());

        EXPECT_EQ(operationResult, OperationResult::Completed);
    }

    TEST_F(SimProcessModuleTest, ProcessThrowsWhenNotInitialized)
    {
        SimProcessModule module = SimProcessModule();

        EXPECT_THROW(module.process(wafer, recipe, std::stop_token()), std::logic_error);
    }

	TEST_F(SimProcessModuleTest, CompletedProcessAllowsAnotherWaferToProcess)
	{
		SimProcessModule module = SimProcessModule();

		module.initialize();

		OperationResult firstResult = module.process(wafer,  recipe, std::stop_token());
		OperationResult secondResult = module.process(wafer2,  recipe, std::stop_token());

		EXPECT_EQ(firstResult, OperationResult::Completed);
		EXPECT_EQ(secondResult, OperationResult::Completed);
	}

	TEST_F(SimProcessModuleTest, CancelledProcessAllowsAnotherWaferToProcess)
	{
		SimProcessModule module = SimProcessModule();

                module.initialize();

		OperationResult firstResult;

		std::jthread processThread([&](std::stop_token stopToken)
		{
			firstResult = module.process(wafer,  longerRecipe, stopToken);
		});

		while (!module.processingWaferId().has_value())
		{
			std::this_thread::yield();
		}

		EXPECT_EQ(module.processingWaferId().value(), wafer.id);

		processThread.request_stop();
		processThread.join();

                OperationResult secondResult = module.process(wafer2,  recipe, std::stop_token());

                EXPECT_EQ(firstResult, OperationResult::Cancelled);
                EXPECT_EQ(secondResult, OperationResult::Completed);
	}

	TEST_F(SimProcessModuleTest, RecoversWhenNotProcessinfWafer)
	{
		SimProcessModule processModule;

		EXPECT_EQ(processModule.recover(), RecoveryResult::Recovered);
	}

	TEST_F(SimProcessModuleTest, RecoveryFailsWhileProcessingWafer)
	{
	    SimProcessModule processModule;

	    processModule.initialize();

	    Wafer wafer{
        	"W1",
	        Location{ Station::LoadPort, 4 }
	    };

	    Recipe recipe{ 1, "Recipe", 0.01};

	    std::jthread worker([&]
	    {
	        processModule.process(
        	    wafer,
            	recipe,
            	{}
        	);
	    });

	    while (!processModule.processingWaferId().has_value())
    	    {
	        std::this_thread::yield();
	    }

	    EXPECT_EQ(
        	processModule.recover(),
        	RecoveryResult::Failed
    	    );
	}
}
