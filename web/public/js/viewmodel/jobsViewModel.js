import { jobsApi } from "../model/jobsApi.js";

export function createJobsViewModel() {
    const state = {
        jobs: [],
        scheduler: null,
        connected: false,
        loading: true,
        busy: false,
        error: null,
        message: "Connecting..."
    };

    const listeners = new Set();

    let timer = null;
    let active = false;
    let refreshing = false;

    function notify() {
        const snapshot = {
            ...state,
            jobs: [...state.jobs],
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
        void refresh();

        timer = setInterval(() => {
            void refresh();
        }, 1000);
    }

    function stop() {
        active = false;
        clearInterval(timer);
        timer = null;
    }

    return {
        subscribe,
        start,
        stop,
        refresh,

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

        submitJob: (waferId, recipeId) =>
            command(
                () => jobsApi.submitJob(waferId, recipeId),
                "Job submitted"
            )
    };
}
