import { miniFabApi } from "../model/miniFabApi.js";

export function createEventLogViewModel(api = miniFabApi) {
    const state = {
        events: [],
        error: null
    };

    const listeners = new Set();
    let timer = null;
    let active = false;
    let refreshing = false;

    function notify() {
        const snapshot = {
            ...state,
            events: [...state.events]
        };

        for (const listener of listeners) {
            listener(snapshot);
        }
    }

    function subscribe(listener) {
        listeners.add(listener);
        listener({
            ...state,
            events: [...state.events]
        });

        return () => listeners.delete(listener);
    }

    async function refresh() {
        if (refreshing) return;

        refreshing = true;

        try {
            const result = await api.getEvents();

            if (!active) return;

            state.events = result.events ?? [];
            state.error = null;
        } catch (error) {
            if (active) state.error = error.message;
        } finally {
            refreshing = false;
            if (active) notify();
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

    return { subscribe, refresh, start, stop };
}

