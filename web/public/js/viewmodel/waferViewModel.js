
export function createWaferViewModel() {
    const state = {
        wafers: [],
        busy: false,
        message: "Loading wafers..."
    };

    const listeners = new Set();

    function subscribe(listener) {
        listeners.add(listener);
        listener(state);

        return () => listeners.delete(listener);
    }

    function notify() {
        for (const listener of listeners) {
            listener(state);
        }
    }

    async function load() {
        try {
            const response = await fetch("/api/wafers");

            if (!response.ok) {
                throw new Error(`HTTP ${response.status}`);
            }

            const data = await response.json();

            state.wafers = data.wafers;
            state.message = "";
        } catch (error) {
            state.message = `Failed to load wafers: ${error.message}`;
        }

        notify();
    }

    async function register(id, slot) {
        state.busy = true;
        state.message = "Registering wafer...";
        notify();

        try {
            const response = await fetch("/api/wafers", {
                method: "POST",
                headers: {
                    "Content-Type": "application/json"
                },
                body: JSON.stringify({
                    id,
                    station: "LoadPort",
                    slot
                })
            });

            const result = await response.json();

            if (!response.ok) {
                throw new Error(
                    result.error ?? `HTTP ${response.status}`
                );
            }

            await load();
            state.message = `Wafer ${id} registered`;
        } catch (error) {
            state.message = `Registration failed: ${error.message}`;
        } finally {
            state.busy = false;
            notify();
        }
    }

    return { subscribe, load, register };
}
