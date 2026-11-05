import { miniFabApi } from "../model/miniFabApi.js";

export function createRunViewModel(api = miniFabApi) {
	let snapshot = null;
	let events = [];
	let lastEventSequenceId = 0;
	let lastEventRefresh = 0;
	let message = "Connecting...";
	let runningCommand = false;
	let runPhase = "none";  // none, accepted, running, finished
	let runMessage = "";
	let statusError = null;
	let eventError = null;
	let refreshing = false;
	let timerId = null;
	let recipes = [];
	let selectedRecipeId = null;
	let recipesLoading = false;
	let recipeError = null;
	let eventsRefreshing = false;

	const subscribers = new Set();

	function getState() {
		return {
			equipmentState: snapshot?.equipmentState ?? "Unknown",
			waferLocation: snapshot?.waferLocation ? `${snapshot.waferLocation.station} / Slot
								${snapshot.waferLocation.slot}`
							      : "Unknown",
			robot: snapshot?.robotWaferId ?? "Empty",
			processModule: snapshot?.processModuleWaferId ?? "Empty",
			message: statusError ?? eventError ?? (runMessage || message),
			runPhase,
			events: events,
			recipes: [...recipes],
			selectedRecipeId,
			recipeError,
			canRun: snapshot?.equipmentState === "Idle"
    				&& !runningCommand
    				&& selectedRecipeId !== null
    				&& !recipesLoading
    				&& recipeError === null,
		};
	}

	function notify() {
		const state = getState();

		for (const subscriber of subscribers) {
			subscriber(state);
		}
	}

async function refresh() {
    if (refreshing) return;

    refreshing = true;

    try {
        const latest = await api.getStatus();

        snapshot = latest;
        statusError = null;

if (runPhase === "accepted") {
    if (latest.equipmentState === "Faulted") {
        runPhase = "finished";
        runMessage = "Wafer run faulted. Check equipment events.";
    } else if (latest.equipmentState !== "Idle") {
        runPhase = "running";
        runMessage = "Wafer processing in progress...";
    }
}
        } else if (runPhase === "running") {
            if (latest.equipmentState === "Faulted") {
                runPhase = "finished";
                runMessage = "Wafer run faulted. Check equipment events.";
            } else if (latest.equipmentState === "Idle") {
                runPhase = "finished";
                runMessage = "Equipment returned to Idle. Check events for the outcome.";
            }
        }

        if (runPhase === "none" && !runMessage) {
            message = "Connected";
        }
    } catch (error) {
        snapshot = null;
        statusError = `Status error: ${error.message}`;
    } finally {
        refreshing = false;
        notify();
    }

    await refreshEvents();
}


async function refreshEvents() {
    if (eventsRefreshing) return;

    const now = Date.now();

    if (now - lastEventRefresh < 1000) return;

    eventsRefreshing = true;
    lastEventRefresh = now;

    try {
        const result = await api.getEvents();
        eventError = null;

        const latestId =
            result.events.at(-1)?.sequenceId ?? 0;

        if (latestId < lastEventSequenceId) {
            events = [];
            lastEventSequenceId = 0;
        }

        const newEvents = result.events.filter(
            event => event.sequenceId > lastEventSequenceId
        );

        events = [...events, ...newEvents];
        lastEventSequenceId = latestId;

        notify();
    } catch (error) {
        eventError = `Event error: ${error.message}`;
        notify();
    } finally {
        eventsRefreshing = false;
    }
}


async function run() {
    if (!getState().canRun) return;

    const recipeId = selectedRecipeId;

    runningCommand = true;
    runPhase = "accepted";
    runMessage = `Submitting recipe ${recipeId}...`;
    notify();

    try {
        const result = await api.runWafer(recipeId);

        if (result.accepted) {
            runMessage = `Run accepted (recipe ${recipeId})`;
        } else {
            runPhase = "finished";
            runMessage =
                `Run rejected: ${result.error ?? `HTTP ${result.httpStatus}`}`;
        }
    } catch (error) {
        runPhase = "finished";
        runMessage = `Run error: ${error.message}`;
    } finally {
        runningCommand = false;
        notify();

        // Request a fresh snapshot after the command response.
        // Polling will continue independently.
        void refresh();
    }
}


	function subscribe(callback) {
		subscribers.add(callback);
		callback(getState());

		return () => subscribers.delete(callback);
	}

	function start() {
		if (timerId !== null) return;

		loadRecipes();
		refresh();
		timerId = setInterval(refresh, 250);
	}

	function stop() {
		clearInterval(timerId);
		timerId = null;
	}

	async function loadRecipes() {
    recipesLoading = true;
    recipeError = null;
    notify();

    try {
        const loaded = await api.getRecipes();

        recipes = loaded;

        if (!recipes.some(r => r.id === selectedRecipeId)) {
            selectedRecipeId = recipes.length > 0
                ? recipes[0].id
                : null;
        }
    } catch (error) {
        recipes = [];
        selectedRecipeId = null;
        recipeError = `Recipe load error: ${error.message}`;
    } finally {
        recipesLoading = false;
        notify();
    }
}

function selectRecipe(id) {
    const recipe = recipes.find(r => r.id === id);

    selectedRecipeId = recipe ? recipe.id : null;
    notify();
}

	return {
		getState,
		subscribe,
		refresh,
		loadRecipes,
		selectRecipe,
		run,
		start,
		stop
	};

}
