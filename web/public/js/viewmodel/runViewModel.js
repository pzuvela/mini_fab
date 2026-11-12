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
	let wafers = [];
	let selectedWaferId = null;
	let wafersLoading = false;
	let waferError = null;
	let eventsRefreshing = false;
	let currentRunId = null;
	let controlCommandPending = false;
	let cancellationPending = false;

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
			events: events.map(event => ({ ...event })),
			recipes: [...recipes],
			selectedRecipeId,
			recipeError,
			wafers: [...wafers],
			selectedWaferId,
			waferError,
			canRun: snapshot?.equipmentState === "Idle"
    				&& !runningCommand
    				&& !controlCommandPending
    				&& selectedRecipeId !== null
    				&& !recipesLoading
    				&& recipeError === null
				&& selectedWaferId !== null
				&& !wafersLoading
				&& waferError === null,
			canCancel: ["Moving", "Processing"].includes(
				snapshot?.equipmentState
			) && !controlCommandPending && !cancellationPending,
			canReset: snapshot?.equipmentState === "Faulted" && !controlCommandPending,
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
        if (!["Moving", "Processing"].includes(latest.equipmentState)) {
 	   cancellationPending = false;
	}

	statusError = null;

	if (runPhase === "accepted") {
    		if (latest.equipmentState === "Faulted") {
        		runMessage = "Wafer run faulted. Check equipment events.";
    		} else if (latest.equipmentState !== "Idle") {
        		runPhase = "running";
        		runMessage = "Wafer processing in progress...";
		}
        } else if (runPhase === "running") {
        	if (latest.equipmentState === "Faulted") {
                	runMessage = "Equipment faulted. Awaiting run outcome...";
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
    try {
        const result = await api.getEvents();
        eventError = null;

        const orderedEvents = [...result.events].sort(
            (a, b) => a.sequenceId - b.sequenceId
        );
        const latestId = orderedEvents.at(-1)?.sequenceId ?? 0;

        if (latestId < lastEventSequenceId) {
            events = [];
            lastEventSequenceId = 0;
        }

        const newEvents = orderedEvents.filter(
            event => event.sequenceId > lastEventSequenceId
        );

        events = [...events, ...newEvents];
        lastEventSequenceId = latestId;
        lastEventRefresh = Date.now();

	for (const event of newEvents) {

		if (event.runId !== currentRunId || currentRunId === null) {
			continue;
		}

    		if (runPhase !== "accepted" && runPhase !== "running") {
        		continue;
    		}

    		switch (event.type) {
        		case "RunCompleted":
            			runPhase = "finished";
            			runMessage = "Wafer run completed successfully.";
            			break;

        		case "RunFailed":
            			runPhase = "finished";
            			runMessage = "Wafer run failed. Check equipment events.";
            			break;

        		case "RunCancelled":
            			runPhase = "finished";
            			runMessage = "Wafer run cancelled.";
            			break;
    		}
	}

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

    currentRunId = null;

    const recipeId = selectedRecipeId;

    const waferId = selectedWaferId;

    runningCommand = true;
    runPhase = "accepted";
    runMessage = `Submitting recipe ${recipeId}...`;
    notify();

    try {
        const result = await api.runWafer(waferId, recipeId);

	if (result.accepted) {
		currentRunId = result.runId;

    		if (runPhase === "accepted") {
        		runMessage = `Run accepted (recipe ${recipeId})`;
    		}
       	} else {
            	runPhase = "finished";
            	runMessage = `Run rejected: ${result.error ?? `HTTP ${result.httpStatus}`}`;
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

		void loadRecipes();
		void refresh();
		void loadWafers();
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

async function cancelRun() {
    if (!getState().canCancel) return;

    controlCommandPending = true;
    notify();

    try {
        const result = await api.cancelRun();
	cancellationPending = result.accepted;
        runMessage = result.accepted
            ? "Cancellation requested. Awaiting run outcome..."
            : `Cancel rejected: ${result.error ?? "Unknown error"}`;
    } catch (error) {
        runMessage = `Cancel error: ${error.message}`;
    } finally {
        controlCommandPending = false;
        notify();
        await refresh();
    }
}

async function resetEquipment() {
    if (!getState().canReset) return;

    controlCommandPending = true;
    notify();

    try {
        const result = await api.resetEquipment();

        runMessage = result.recovered
            ? "Equipment recovered successfully."
            : `Reset failed: ${result.error ?? "Unknown error"}`;
    } catch (error) {
        runMessage = `Reset error: ${error.message}`;
    } finally {
        controlCommandPending = false;
        notify();
        await refresh();
    }
}

function selectRecipe(id) {
    const recipe = recipes.find(r => r.id === id);

    selectedRecipeId = recipe ? recipe.id : null;
    notify();
}

async function loadWafers() {
    wafersLoading = true;
    waferError = null;
    notify();

    try {
        const data = await api.getWafers();

        wafers = data.wafers.filter(
            wafer => wafer.location.station === "LoadPort"
        );

        if (!wafers.some(w => w.id === selectedWaferId)) {
            selectedWaferId = wafers[0]?.id ?? null;
        }
    } catch (error) {
        wafers = [];
        selectedWaferId = null;
        waferError = `Wafer load error: ${error.message}`;
    } finally {
        wafersLoading = false;
        notify();
    }
}

function selectWafer(id) {
    selectedWaferId = wafers.some(w => w.id === id)
        ? id
        : null;

    notify();
}



	return {
		getState,
		subscribe,
		refresh,
		refreshEvents,
		loadRecipes,
		selectRecipe,
		loadWafers,
		selectWafer,
		run,
		start,
		stop,
		cancelRun,
		resetEquipment
	};

}

