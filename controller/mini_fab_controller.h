#pragma once

#include <vector>
#include <optional>

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

public:
	explicit MiniFabController(IEventRepository* eventRepository = nullptr);
	explicit MiniFabController(RecipeManager& recipeManager);

	MiniFabController(RecipeManager& recipeManager, IEventRepository& eventReposutory);

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

	bool dispatchNextJob();

	bool waitForActiveJobCompletion();

	std::size_t pendingJobCount() const;
};
