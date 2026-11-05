#include <chrono>
#include <stdexcept>

#include "mini_fab_controller.h"
#include "domain/wafer.h"
#include "domain/location.h"
#include "domain/station.h"


MiniFabController::MiniFabController(
	IEventRepository* eventRepository
)
	: robot_(std::chrono::milliseconds{300}), eventLog_({}, eventRepository), engine_(manager_, robot_, processModule_, eventLog_)
{
	manager_.addWafer(
		Wafer{
			.id = "W1",
			.location = Location{ Station::LoadPort, 3 }
		}
	);

	eventLog_.record(
		EventType::EquipmentStarted,
		"MiniFab equipment initialzed"
	);

	eventLog_.record(
		EventType::WaferRegistered,
		"Wafer W1 registered at LoadPort / Slot 3"
	);

}

MiniFabController::MiniFabController(RecipeManager& recipeManager)
	: MiniFabController()
{
	recipeManager_ = &recipeManager;
}

MiniFabController::MiniFabController(
    RecipeManager& recipeManager,
    IEventRepository& eventRepository
)
    : MiniFabController(&eventRepository)
{
    recipeManager_ = &recipeManager;
}

MiniFabController::~MiniFabController()
{
	stopScheduler();
}

EquipmentSnapshot MiniFabController::status() const
{
	return EquipmentSnapshot{
		.state = engine_.state(),
		.waferLocation = manager_.getLocation("W1"),
		.robotWaferId = robot_.carriedWaferId(),
		.processModuleWaferId = processModule_.processingWaferId()
	};
}

StartResult MiniFabController::runWafer(const Recipe& recipe, std::uint64_t* acceptedRunId)
{
	return engine_.startProcessWafer(
		"W1",
		Location{Station::ProcessModule, 1},
		recipe,
		acceptedRunId
	);
}

std::vector<EquipmentEvent> MiniFabController::events() const
{
	return eventLog_.getEvents();
}

StartResult MiniFabController::runWafer(int recipeId, std::uint64_t* acceptedRunId)
{
    if (!recipeManager_)
        return StartResult::Rejected;

    auto recipe = recipeManager_->getById(recipeId);

    if (!recipe.has_value())
        return StartResult::Rejected;

    return runWafer(*recipe, acceptedRunId);
}

std::vector<Recipe> MiniFabController::getRecipes() const
{
    if (!recipeManager_)
        return {};

    return recipeManager_->getAll();
}

std::optional<Recipe> MiniFabController::getRecipe(int id) const
{
    if (!recipeManager_)
        return std::nullopt;

    return recipeManager_->getById(id);
}

bool MiniFabController::addRecipe(const Recipe& recipe)
{
    return recipeManager_ && recipeManager_->add(recipe);
}

bool MiniFabController::updateRecipe(const Recipe& recipe)
{
    return recipeManager_ && recipeManager_->update(recipe);
}

bool MiniFabController::removeRecipe(int id)
{
    return recipeManager_ && recipeManager_->remove(id);
}

RecipeUpsertResult MiniFabController::upsertRecipe(
    const Recipe& recipe)
{
    if (!recipeManager_)
        return RecipeUpsertResult::Failed;

    return recipeManager_->upsert(recipe);
}

StateTransition MiniFabController::cancelRun()
{
    return engine_.cancel();
}

RecoveryResult MiniFabController::resetEquipment()
{
    return engine_.reset();
}

std::uint64_t MiniFabController::enqueueJob(const std::string& waferId, int recipeId, int processModuleId)
{
	std::uint64_t jobId;

	{
		std::lock_guard lock(schedulerMutex_);

		jobId = scheduler_.enqueue(
			waferId,
			recipeId,
			processModuleId
		);
	}

	schedulerCv_.notify_one();

	return jobId;
}

StartResult MiniFabController::runWafer(
	const std::string& waferId,
	int recipeId,
	int processModuleId,
	std::uint64_t* acceptedRunId
)
{
	if (!recipeManager_) return StartResult::Rejected;

	auto recipe = recipeManager_->getById(recipeId);

	if (!recipe.has_value()) return StartResult::Rejected;

	if (processModuleId != 1) return StartResult::Rejected;

	return engine_.startProcessWafer(waferId, Location{ Station::ProcessModule, processModuleId }, recipe.value(), acceptedRunId);
}

bool MiniFabController::dispatchNextJob()
{
	std::lock_guard<std::mutex> lock(schedulerMutex_);

	return scheduler_.dispatchNext(
		[this](const Job& job)
		{
			return runWafer(job.waferId, job.recipeId, job.processModuleId) == StartResult::Accepted;
		}
	);
}

bool MiniFabController::waitForActiveJobCompletion()
{
	std::optional<Job> activeJob;

	{
		std::lock_guard<std::mutex> lock(schedulerMutex_);
		activeJob = scheduler_.activeJob();
	}

	if (!activeJob) return false;

	engine_.waitForCompletion();

	{
		std::lock_guard<std::mutex> lock(schedulerMutex_);
		return scheduler_.completeActiveJob(activeJob->id);
	}
}

std::size_t MiniFabController::pendingJobCount() const
{
	std::lock_guard<std::mutex> lock(schedulerMutex_);
	return scheduler_.pendingCount();
}

void MiniFabController::startScheduler()
{
	if (schedulerWorker_.joinable()) return;

	schedulerWorker_ = std::jthread(
		[this](std::stop_token stopToken)
		{
			schedulerLoop(stopToken);
		}
	);
}

void MiniFabController::stopScheduler()
{
	if (!schedulerWorker_.joinable()) return;

	schedulerWorker_.request_stop();
	schedulerCv_.notify_all();
	schedulerWorker_.join();
}

void MiniFabController::schedulerLoop(std::stop_token stopToken)
{
	std::unique_lock<std::mutex> lock(schedulerMutex_);

	while (!stopToken.stop_requested())
	{
		{

			bool hasWork = schedulerCv_.wait(
				lock,
				stopToken,
				[this]()
				{
					return scheduler_.pendingCount() > 0;
				}
			);

			if (!hasWork || stopToken.stop_requested()) break;
		}

		if (!dispatchNextJob())
		{
			break;
		}

		waitForActiveJobCompletion();
	}
}
