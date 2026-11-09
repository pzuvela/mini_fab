
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


import test from "node:test";
import assert from "node:assert/strict";

import { createWaferViewModel } from
    "../../web/public/js/viewmodel/waferViewModel.js";

function response(status, body) {
    return {
        ok: status >= 200 && status < 300,
        status,
        json: async () => body
    };
}

function setup(fetchMock) {
    const originalFetch = globalThis.fetch;
    globalThis.fetch = fetchMock;

    const vm = createWaferViewModel();
    let state;

    vm.subscribe(value => {
        state = { ...value, wafers: [...value.wafers] };
    });

    return {
        vm,
        get state() { return state; },
        restore() { globalThis.fetch = originalFetch; }
    };
}

test("loads wafer inventory", async () => {
    const ctx = setup(async () => response(200, {
        wafers: [
            { id: "W1", location: { station: "LoadPort", slot: 3 } },
            { id: "W2", location: { station: "LoadPort", slot: 4 } }
        ]
    }));

    try {
        await ctx.vm.load();

        assert.equal(ctx.state.wafers.length, 2);
        assert.equal(ctx.state.wafers[1].id, "W2");
        assert.equal(ctx.state.message, "");
    } finally {
        ctx.restore();
    }
});

test("reports inventory loading failure", async () => {
    const ctx = setup(async () => {
        throw new Error("Connection refused");
    });

    try {
        await ctx.vm.load();

        assert.match(ctx.state.message, /Failed to load/);
        assert.match(ctx.state.message, /Connection refused/);
    } finally {
        ctx.restore();
    }
});

test("registers wafer and refreshes inventory", async () => {
    const calls = [];

    const ctx = setup(async (url, options) => {
        calls.push({ url, options });

        if (options?.method === "POST")
            return response(201, { status: "registered" });

        return response(200, {
            wafers: [
                { id: "W1", location: { station: "LoadPort", slot: 3 } },
                { id: "W2", location: { station: "LoadPort", slot: 4 } }
            ]
        });
    });

    try {
        await ctx.vm.register("W2", 4);

        assert.equal(calls[0].url, "/api/wafers");
        assert.equal(calls[0].options.method, "POST");

        assert.deepEqual(
            JSON.parse(calls[0].options.body),
            { id: "W2", station: "LoadPort", slot: 4 }
        );

        assert.equal(ctx.state.wafers.length, 2);
        assert.equal(ctx.state.busy, false);
        assert.match(ctx.state.message, /W2 registered/);
    } finally {
        ctx.restore();
    }
});

test("registration conflict preserves inventory", async () => {
    const ctx = setup(async () => response(409, {
        error: "Slot already occupied"
    }));

    try {
        await ctx.vm.register("W2", 3);

        assert.equal(ctx.state.wafers.length, 0);
        assert.equal(ctx.state.busy, false);
        assert.match(ctx.state.message, /Slot already occupied/);
    } finally {
        ctx.restore();
    }
});

test("prevents duplicate submissions", async () => {
    let resolvePost;
    let postCount = 0;

    const ctx = setup((url, options) => {
        if (options?.method === "POST") {
            postCount++;

            return new Promise(resolve => {
                resolvePost = resolve;
            });
        }

        return Promise.resolve(response(200, { wafers: [] }));
    });

    try {
        const first = ctx.vm.register("W2", 4);
        const second = ctx.vm.register("W2", 4);

        assert.equal(postCount, 1);

        resolvePost(response(201, { status: "registered" }));

        await Promise.all([first, second]);
        assert.equal(ctx.state.busy, false);
    } finally {
        ctx.restore();
    }
});

test("ignores stale inventory responses", async () => {
    const resolvers = [];

    const ctx = setup(() => new Promise(resolve => {
        resolvers.push(resolve);
    }));

    try {
        const first = ctx.vm.load();
        const second = ctx.vm.load();

        resolvers[1](response(200, {
            wafers: [{ id: "W2", location: { station: "LoadPort", slot: 4 } }]
        }));

        await second;

        resolvers[0](response(200, {
            wafers: [{ id: "W1", location: { station: "LoadPort", slot: 3 } }]
        }));

        await first;

        assert.equal(ctx.state.wafers[0].id, "W2");
    } finally {
        ctx.restore();
    }
});

