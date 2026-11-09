
import test from "node:test";
import assert from "node:assert/strict";

import { createWaferViewModel } from
    "../../web/public/js/viewmodel/waferViewModel.js";

function mockFetch(handler) {
    const original = globalThis.fetch;
    globalThis.fetch = handler;

    return () => {
        globalThis.fetch = original;
    };
}

function jsonResponse(status, data) {
    return {
        ok: status >= 200 && status < 300,
        status,
        async json() {
            return data;
        }
    };
}

const initialWafers = [
    {
        id: "W1",
        location: { station: "LoadPort", slot: 3 }
    }
];

test("Loads registered wafers", async () => {
    const restore = mockFetch(async (url) => {
        assert.equal(url, "/api/wafers");
        return jsonResponse(200, { wafers: initialWafers });
    });

    try {
        const vm = createWaferViewModel();
        let state;

        vm.subscribe(s => { state = s; });
        await vm.load();

        assert.deepEqual(state.wafers, initialWafers);
        assert.equal(state.message, "");
    } finally {
        restore();
    }
});

test("Registers wafer and refreshes inventory", async () => {
    const calls = [];

    const restore = mockFetch(async (url, options) => {
        calls.push({ url, options });

        if (options?.method === "POST") {
            assert.deepEqual(JSON.parse(options.body), {
                id: "W2",
                station: "LoadPort",
                slot: 4
            });

            return jsonResponse(201, {
                status: "registered"
            });
        }

        return jsonResponse(200, {
            wafers: [
                ...initialWafers,
                {
                    id: "W2",
                    location: {
                        station: "LoadPort",
                        slot: 4
                    }
                }
            ]
        });
    });

    try {
        const vm = createWaferViewModel();
        let state;

        vm.subscribe(s => { state = s; });

        await vm.register("W2", 4);

        assert.equal(calls.length, 2);
        assert.equal(calls[0].options.method, "POST");
        assert.equal(calls[1].url, "/api/wafers");
        assert.equal(state.wafers.length, 2);
        assert.equal(state.busy, false);
        assert.match(state.message, /registered/i);
    } finally {
        restore();
    }
});

test("Displays duplicate wafer error", async () => {
    const restore = mockFetch(async () =>
        jsonResponse(409, {
            error: "Wafer ID already exists"
        })
    );

    try {
        const vm = createWaferViewModel();
        let state;

        vm.subscribe(s => { state = s; });

        await vm.register("W1", 4);

        assert.match(
            state.message,
            /Wafer ID already exists/
        );
        assert.equal(state.busy, false);
    } finally {
        restore();
    }
});

test("Displays occupied slot error", async () => {
    const restore = mockFetch(async () =>
        jsonResponse(409, {
            error: "Slot already occupied"
        })
    );

    try {
        const vm = createWaferViewModel();
        let state;

        vm.subscribe(s => { state = s; });

        await vm.register("W2", 3);

        assert.match(state.message, /Slot already occupied/);
        assert.equal(state.busy, false);
    } finally {
        restore();
    }
});

test("Handles inventory loading failure", async () => {
    const restore = mockFetch(async () =>
        jsonResponse(503, {})
    );

    try {
        const vm = createWaferViewModel();
        let state;

        vm.subscribe(s => { state = s; });
        await vm.load();

        assert.match(state.message, /HTTP 503/);
        assert.deepEqual(state.wafers, []);
    } finally {
        restore();
    }
});
