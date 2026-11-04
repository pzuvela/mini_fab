import { miniFabApi } from "../model/miniFabApi.js";

export function createRunViewModel(api = miniFabApi) {
	let snapshot = null;
	let events = [];
	let lastEventSequenceId = 0;
	let lastEventRefresh = 0;
	let message = "Connecting...";
	let runningCommand = false;
	let refreshing = false;
	let timerId = null;
	let recipes = [];
	let selectedRecipeId = null;
	let recipesLoading = false;
	let recipeError = null;

	const subscribers = new Set();

	function getState() {
		return {
			equipmentState: snapshot?.equipmentState ?? "Unknown",
			waferLocation: snapshot?.waferLocation ? `${snapshot.waferLocation.station} / Slot
								${snapshot.waferLocation.slot}`
							      : "Unknown",
			robot: snapshot?.robotWaferId ?? "Empty",
			processModule: snapshot?.processModuleWaferId ?? "Empty",
			message,
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
			snapshot = await api.getStatus();
			message = "Connected";
		} catch(error) {
			snapshot = null;
			message = `Status error: ${error.message}`;
		} finally {
			refreshing = false;
			notify();
		}

		await refreshEvents();
	}

	async function refreshEvents() {
		const now = Date.now();

		if (now - lastEventRefresh < 1000) return;

		lastEventRefresh = now;

		try {
			const result = await api.getEvents();

			const latestId = result.events.at(-1)?.sequenceId ?? 0;

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
			console.error("Event refresh failed:", error);
		}

	}

async function run() {
    if (!getState().canRun) return;

    const recipeId = selectedRecipeId;

    runningCommand = true;
    notify();

    try {
        const result = await api.runWafer(recipeId);

        message = result.accepted
            ? `Wafer run accepted (recipe ${recipeId})`
            : `Run rejected: ${result.error ?? `HTTP ${result.httpStatus}`}`;

        await refresh();
    } catch (error) {
        message = `Run error: ${error.message}`;
    } finally {
        runningCommand = false;
        notify();
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
