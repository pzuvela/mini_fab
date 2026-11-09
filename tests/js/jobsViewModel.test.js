import test from "node:test";
import assert from "node:assert/strict";

import { createJobsViewModel } from  "../../web/public/js/viewmodel/jobsViewModel.js";

function createMockApi(wafers = []) {
    let wafers = initialWafers;
    const submissions = [];

    const api = {
        async getWafers() {
            return { wafers };
        },

        async getRecipes() {
            return [
                { id: 1, name: "Test Recipe" }
            ];
        },

        async getJobs() {
            return [];
        },

        async getScheduler() {
            return {
                state: "Stopped",
                pendingJobs: 0,
                activeJobId: null
            };
        },

        async getEvents() {
            return { events: [] };
        },

        async submitJob(waferId, recipeId) {
            submissions.push({ waferId, recipeId });
            return { jobId: submissions.length };
        }
    };

    return { api, submissions, setWafers(newWafers) { wafers = newWafers; } };
}

const wafer = (id, station = "LoadPort") => ({
    id,
    location: { station, slot: 1 }
});

// Allow asynchronous startup operations to settle.
async function prepareViewModel(mock) {
    const vm = createJobsViewModel(mock.api);

    let state;
    vm.subscribe(snapshot => {
        state = snapshot;
    });

    vm.start();

    // Explicitly await the wafer and recipe loading.
    await Promise.all([
        vm.loadWafers(),
        vm.loadRecipes()
    ]);

    return {
        vm,
        getState: () => state,
        stop: () => vm.stop()
    };
}

test("Loads only available wafers", async () => {
    const mock = createMockApi([
        wafer("W1"),
        wafer("W2"),
        wafer("W3", "ProcessModule")
    ]);

    const ctx = await prepareViewModel(mock);

    try {
        assert.deepEqual(
            ctx.getState().wafers.map(w => w.id),
            ["W1", "W2"]
        );

        assert.equal(
            ctx.getState().selectedWaferId,
            "W1"
        );
    } finally {
        ctx.stop();
    }
});

test("Empty inventory prevents submission", async () => {
    const mock = createMockApi([]);
    const ctx = await prepareViewModel(mock);

    try {
        assert.equal(
            ctx.getState().selectedWaferId,
            null
        );

        await ctx.vm.submitJob();

        assert.equal(mock.submissions.length, 0);
    } finally {
        ctx.stop();
    }
});

test("Selecting a wafer updates selection", async () => {
    const mock = createMockApi([
        wafer("W1"),
        wafer("W2")
    ]);

    const ctx = await prepareViewModel(mock);

    try {
        ctx.vm.selectWafer("W2");

        assert.equal(
            ctx.getState().selectedWaferId,
            "W2"
        );
    } finally {
        ctx.stop();
    }
});

test("Unknown wafer cannot be selected", async () => {
    const mock = createMockApi([
        wafer("W1")
    ]);

    const ctx = await prepareViewModel(mock);

    try {
        ctx.vm.selectWafer("UNKNOWN");

        assert.equal(
            ctx.getState().selectedWaferId,
            null
        );
    } finally {
        ctx.stop();
    }
});

test("Job submits selected wafer and recipe", async () => {
    const mock = createMockApi([
        wafer("W1"),
        wafer("W2")
    ]);

    const ctx = await prepareViewModel(mock);

    try {
        ctx.vm.selectWafer("W2");

        await ctx.vm.submitJob();

        assert.deepEqual(mock.submissions, [
            { waferId: "W2", recipeId: 1 }
        ]);
    } finally {
        ctx.stop();
    }
});


test("Selection updates when wafer becomes unavailable", async () => {
    const mock = createMockApi([
        wafer("W1"),
        wafer("W2")
    ]);

    const ctx = await prepareViewModel(mock);

    try {
        ctx.vm.selectWafer("W2");

        assert.equal(ctx.getState().selectedWaferId, "W2");

        mock.setWafers([
            wafer("W1"),
            wafer("W2", "ProcessModule")
        ]);

        await ctx.vm.loadWafers();

        assert.deepEqual(
            ctx.getState().wafers.map(w => w.id),
            ["W1"]
        );

        assert.equal(
            ctx.getState().selectedWaferId,
            "W1"
        );
    } finally {
        ctx.stop();
    }
});

test("Refresh preserves valid wafer selection", async () => {
    const mock = createMockApi([
        wafer("W1"),
        wafer("W2")
    ]);

    const ctx = await prepareViewModel(mock);

    try {
        ctx.vm.selectWafer("W2");

        mock.setWafers([
            wafer("W1"),
            wafer("W2"),
            wafer("W3")
        ]);

        await ctx.vm.loadWafers();

        assert.equal(
            ctx.getState().selectedWaferId,
            "W2"
        );

        assert.equal(
            ctx.getState().wafers.length,
            3
        );
    } finally {
        ctx.stop();
    }
});


