#pragma once

#include <vector>
#include <optional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <cstdint>
#include <string>
#include <vector>

#include "equipment_snapshot.h"
#include "wafer/wafer_manager.h"
#include "equipment/robot/sim_robot.h"
#include "equipment/process_module/sim_process_module.h"
#include "engine/process_engine.h"
#include "equipment/operation_result.h"
#include "equipment/event_log.h"
#include "equipment/equipment_event.h"
#include "recipes/recipe_manager.h"
#include "recipes/recipe_upsert_result.h"
#include "engine/start_result.h"
#include "persistence/ievent_repository.h"
#include "engine/state_transition.h"
#include "equipment/recovery_result.h"
#include "scheduler/job_scheduler.h"
#include "scheduler/dispatch_result.h"
#include "completion_owner.h"
#include "scheduler/scheduler_state.h"
#include "scheduler/scheduler_snapshot.h"
#include "scheduler/job_snapshot.h"
#include "wafer/wafer_registration_result.h"
#include "wafer/iwafer_repository.h"


class MiniFabController
{
private:
	// declaration order is important, engine last, so that it gets destroyed first,
	// allowing its thread to finish before components it references are destroyed
	WaferManager manager_;
	SimRobot robot_;
	SimProcessModule processModule_;
	EventLog eventLog_;
	ProcessEngine engine_;
	RecipeManager* recipeManager_ = nullptr;
	JobScheduler scheduler_;
	mutable std::mutex schedulerMutex_;
	std::condition_variable_any schedulerCv_;
	mutable std::mutex schedulerLifecycleMutex_;
	std::jthread schedulerWorker_;
	std::uint64_t schedulerRevision_ = 0;
	CompletionOwner completionOwner_ = CompletionOwner::None;
	bool completionClaimed_ = false;
	bool schedulerFaulted_ = false;
	std::string schedulerError_;

	void schedulerLoop(std::stop_token stopToken);
	DispatchResult dispatchNextJobImpl(CompletionOwner owner);
	bool waitForActiveJobCompletionImpl(CompletionOwner caller);

protected:
	virtual OperationResult awaitEngineCompletion();

public:
	explicit MiniFabController(IEventRepository* eventRepository = nullptr);
	explicit MiniFabController(RecipeManager& recipeManager);

	MiniFabController(RecipeManager& recipeManager, IEventRepository& eventRepository);

	MiniFabController(RecipeManager& recipeManager, IEventRepository& eventRepository, IWaferRepository& waferRepository);

	virtual ~MiniFabController();

	StartResult runWafer(const Recipe& recipe, std::uint64_t* acceptedRunId = nullptr);
	StartResult runWafer(int RecipeId, std::uint64_t* acceptedRunId = nullptr);

	StartResult runWafer(
		const std::string& waferId,
		int recipeId,
		int processModuleId,
		std::uint64_t* acceptedRunId = nullptr
	);

	std::vector<Recipe> getRecipes() const;
	std::optional<Recipe> getRecipe(int id) const;

	bool addRecipe(const Recipe& recipe);
	bool updateRecipe(const Recipe& recipe);
	bool removeRecipe(int id);
	RecipeUpsertResult upsertRecipe(const Recipe& recipe);

	EquipmentSnapshot status() const;

	std::vector<EquipmentEvent> events() const;

	StateTransition cancelRun();
	RecoveryResult resetEquipment();

	std::uint64_t enqueueJob(const std::string& wfaerId, int recipeId, int processModuleId);

	DispatchResult dispatchNextJob();

	bool waitForActiveJobCompletion();

	std::size_t pendingJobCount() const;

	void startScheduler();
	void stopScheduler();

	std::optional<JobStatus> jobStatus(std::uint64_t jobId) const;

	bool isValidJob(const Job& job) const;

	SchedulerState schedulerState() const;
	std::string schedulerError() const;
	bool resetSchedulerFault();
	SchedulerSnapshot schedulerSnapshot() const;

	std::vector<JobSnapshot> jobsSnapshot() const;

	std::vector<Wafer> wafersSnapshot() const;
	WaferRegistrationResult registerWafer(const Wafer& wafer);
};
