import { jobsApi } from "../model/jobsApi.js";

export function createJobsViewModel() {
    const state = {
        jobs: [],
        events: [],
        recipes: [],
        selectedRecipeId: null,
        recipeError: null,
        eventError: null,
        scheduler: null,
        connected: false,
        loading: true,
        busy: false,
        error: null,
        message: "Connecting...",
	wafers: [],
	selectedWaferId: null,
	waferError: null,
    };

    const listeners = new Set();

    let timer = null;
    let active = false;
    let refreshing = false;

    function notify() {
        const snapshot = {
            ...state,
            jobs: [...state.jobs],
            events: [...state.events],
            recipes: [...state.recipes],
	    wafers: [...state.wafers],
            scheduler: state.scheduler
                ? { ...state.scheduler }
                : null
        };

        for (const listener of listeners) {
            listener(snapshot);
        }
    }

    function subscribe(listener) {
        listeners.add(listener);
        listener({
            ...state,
            jobs: [...state.jobs],
            events: [...state.events],
            recipes: [...state.recipes],
	    wafers: [...state.wafers],
            scheduler: state.scheduler
                ? { ...state.scheduler }
                : null
        });

        return () => listeners.delete(listener);
    }

    async function refresh() {
        if (refreshing) return;

        refreshing = true;

        try {
            const [jobs, scheduler] = await Promise.all([
                jobsApi.getJobs(),
                jobsApi.getScheduler()
            ]);

            if (!active) return;

            state.jobs = jobs;
            state.scheduler = scheduler;
            state.connected = true;
            state.error = null;
            state.message = "Connected";
        } catch (error) {
            if (!active) return;

            state.connected = false;
            state.error = error.message;
            state.message = `Connection error: ${error.message}`;
        } finally {
            refreshing = false;
            state.loading = false;

            if (active) notify();
        }
    }

    async function refreshEvents() {
        try {
            const data = await jobsApi.getEvents();
            if (!active) return;
            state.events = data.events ?? [];
            state.eventError = null;
        } catch (error) {
            if (active) state.eventError = error.message;
        } finally {
            if (active) notify();
        }
    }

    async function loadRecipes() {
        try {
            const recipes = await jobsApi.getRecipes();
            if (!active) return;
            state.recipes = recipes;
            state.recipeError = null;
            if (!recipes.some(r => r.id === state.selectedRecipeId))
                state.selectedRecipeId = recipes[0]?.id ?? null;
        } catch (error) {
            if (!active) return;
            state.recipes = [];
            state.selectedRecipeId = null;
            state.recipeError = error.message;
        } finally {
            if (active) notify();
        }
    }

    function selectRecipe(id) {
        state.selectedRecipeId = state.recipes.some(r => r.id === id) ? id : null;
        notify();
    }

    async function command(action, successMessage) {
        if (state.busy) return;

        state.busy = true;
        state.error = null;
        notify();

        try {
            await action();
            state.message = successMessage;

            // Wait for any in-flight poll to finish.
            // The next poll will pick up the command result.
            await refresh();
        } catch (error) {
            state.error = error.message;
            state.message = `Command failed: ${error.message}`;
        } finally {
            state.busy = false;
            notify();
        }
    }

    function start() {
        if (active) return;

        active = true;
        void loadRecipes();
        void refresh();
        void refreshEvents();

        timer = setInterval(() => {
            void refresh();
            void refreshEvents();
        }, 1000);
    }

    function stop() {
        active = false;
        clearInterval(timer);
        timer = null;
    }

async function loadWafers() {
    try {
        const data = await jobsApi.getWafers();

        if (!active) return;

        state.wafers = data.wafers.filter(
            wafer => wafer.location.station === "LoadPort"
        );

        state.waferError = null;

        if (!state.wafers.some(
            wafer => wafer.id === state.selectedWaferId
        )) {
            state.selectedWaferId =
                state.wafers[0]?.id ?? null;
        }
    } catch (error) {
        if (!active) return;

        state.waferError = error.message;
        state.wafers = [];
        state.selectedWaferId = null;
    } finally {
        if (active) notify();
    }
}

function selectWafer(id) {
    state.selectedWaferId =
        state.wafers.some(wafer => wafer.id === id)
            ? id
            : null;

    notify();
}

    return {
        subscribe,
        start,
        stop,
        refresh,
        selectRecipe,
        loadRecipes,

        startScheduler: () =>
            command(
                () => jobsApi.startScheduler(),
                "Scheduler start requested"
            ),

        stopScheduler: () =>
            command(
                () => jobsApi.stopScheduler(),
                "Scheduler stopped"
            ),

        resetScheduler: () =>
            command(
                () => jobsApi.resetScheduler(),
                "Scheduler fault reset"
            ),

submitJob: () => {
    if (state.selectedRecipeId === null ||
        state.selectedWaferId === null) return;

    return command(
        () => jobsApi.submitJob(
            state.selectedWaferId,
            state.selectedRecipeId
        ),
        "Job submitted"
    );
}

    };
}
