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
	const auto result = engine_.reset();

	if (result == RecoveryResult::Recovered)
	{
		{
			std::lock_guard<std::mutex> lock(schedulerMutex_);
			++schedulerRevision_;
		}

		schedulerCv_.notify_all();
	}

	return result;
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

DispatchResult MiniFabController::dispatchNextJob()
{
	return dispatchNextJobImpl(CompletionOwner::Manual);
}


DispatchResult MiniFabController::dispatchNextJobImpl(CompletionOwner owner)
{
	std::lock_guard<std::mutex> lock(schedulerMutex_);

	if (schedulerFaulted_) return DispatchResult::Busy;

	if (scheduler_.activeJob().has_value()) return DispatchResult::Busy;

	const auto nextJob = scheduler_.nextQueuedJob();

	if (!nextJob) return DispatchResult::Empty;

	if (!isValidJob(*nextJob))
	{
		scheduler_.rejectNextJob();
		return DispatchResult::Rejected;
	}

	bool started = scheduler_.dispatchNext(
		[this](const Job& job)
		{
			return runWafer(job.waferId, job.recipeId, job.processModuleId) == StartResult::Accepted;
		}
	);

	if (!started) return DispatchResult::Busy;

	completionOwner_ = owner;
	completionClaimed_ = false;

	return DispatchResult::Started;
}

bool MiniFabController::waitForActiveJobCompletion()
{
	return waitForActiveJobCompletionImpl(CompletionOwner::Manual);
}

bool MiniFabController::waitForActiveJobCompletionImpl(CompletionOwner caller)
{
	std::optional<Job> activeJob;

	{
		std::lock_guard<std::mutex> lock(schedulerMutex_);
		activeJob = scheduler_.activeJob();

		if (!activeJob) return false;

		if (completionOwner_ != caller) return false;

		if (completionClaimed_) return false;

		completionClaimed_ = true;
	}

	OperationResult result = OperationResult::Failed;

	std::string error;

	try
	{
		result = awaitEngineCompletion();
	}
	catch (const std::exception& ex)
	{
		error = ex.what();
	}
	catch (...)
	{
		error = "Unknown exception during process completion";
	}

	bool released = false;

	{
		std::lock_guard<std::mutex> lock(schedulerMutex_);
		released = scheduler_.finishActiveJob(activeJob->id, result);

		if (!error.empty())
		{
			schedulerFaulted_ = true;
			schedulerError_ = error;
		}

		if (released)
		{
			completionOwner_ = CompletionOwner::None;
			completionClaimed_ = false;
			++schedulerRevision_;
		}
	}

	schedulerCv_.notify_all();

	return released && error.empty() && result == OperationResult::Completed;
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
	while (!stopToken.stop_requested())
	{
		std::uint64_t observedRevision;

		{
			std::unique_lock<std::mutex> lock(schedulerMutex_);

			const bool hasWork = schedulerCv_.wait(
				lock,
				stopToken,
				[this]()
				{
					return scheduler_.pendingCount() > 0;
				}
			);

			if (!hasWork || stopToken.stop_requested()) break;

			if (schedulerFaulted_) break;

			observedRevision = schedulerRevision_;
		}

		switch (dispatchNextJobImpl(CompletionOwner::Scheduler))
		{
			case DispatchResult::Started:
				waitForActiveJobCompletionImpl(CompletionOwner::Scheduler);
				break;

			case DispatchResult::Rejected:
				continue;

			case DispatchResult::Busy:
			{
				std::unique_lock<std::mutex> lock(schedulerMutex_);

				schedulerCv_.wait(
					lock,
					stopToken,
					[this, observedRevision]()
					{
						return schedulerRevision_ != observedRevision;
					}
				);

				break;
			}

			case DispatchResult::Empty:
				break;
		}

		{
			std::lock_guard<std::mutex> lock(schedulerMutex_);
			if (schedulerFaulted_) break;
		}
	}
}

std::optional<JobStatus> MiniFabController::jobStatus(std::uint64_t jobId) const
{
	std::lock_guard<std::mutex> lock(schedulerMutex_);
	return scheduler_.jobStatus(jobId);
}

bool MiniFabController::isValidJob(const Job& job) const
{
	if (!recipeManager_) return false;

	if (job.processModuleId != 1) return false;

	if (!manager_.getLocation(job.waferId).has_value()) return false;

	return recipeManager_->getById(job.recipeId).has_value();
}

SchedulerState MiniFabController::schedulerState() const
{
	std::lock_guard<std::mutex> lock(schedulerMutex_);

	if (schedulerFaulted_) return SchedulerState::Faulted;

	return schedulerWorker_.joinable() ? SchedulerState::Running : SchedulerState::Stopped;
}

std::string MiniFabController::schedulerError() const
{
	std::lock_guard<std::mutex> lock(schedulerMutex_);
	return schedulerError_;
}

bool MiniFabController::resetSchedulerFault()
{
    {
        std::lock_guard lock(schedulerMutex_);

        if (!schedulerFaulted_) return false;

        if (scheduler_.activeJob()) return false;

        if (engine_.state() != EquipmentState::Idle) return false;

        schedulerFaulted_ = false;
        schedulerError_.clear();
        ++schedulerRevision_;
    }

    schedulerCv_.notify_all();
    return true;
}

OperationResult MiniFabController::awaitEngineCompletion()
{
	return engine_.waitForCompletion();
}
