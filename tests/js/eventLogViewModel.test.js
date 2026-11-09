
import test from "node:test";
import assert from "node:assert/strict";

import { createEventLogViewModel } from "../../web/public/js/viewmodel/eventLogViewModel.js";

const tick = () => new Promise(resolve => setImmediate(resolve));

function deferred() {
    let resolve;
    let reject;

    const promise = new Promise((res, rej) => {
        resolve = res;
        reject = rej;
    });

    return { promise, resolve, reject };
}

test("loads events successfully", async () => {
    const events = [
        {
            sequenceId: 1,
            type: "WaferRegistered",
            severity: "Info",
            timestamp: "2026-11-09T10:00:00Z",
            message: "W2 registered"
        }
    ];

    const api = {
        getEvents: async () => ({ events })
    };

    const vm = createEventLogViewModel(api);
    let state;

    vm.subscribe(s => { state = s; });

    vm.start();
    await tick();

    assert.deepEqual(state.events, events);
    assert.equal(state.error, null);

    vm.stop();
});

test("handles HTTP errors without losing previous events", async () => {
    let fail = false;

    const api = {
        getEvents: async () => {
            if (fail) throw new Error("HTTP 500");
            return { events: [{ sequenceId: 1 }] };
        }
    };

    const vm = createEventLogViewModel(api);
    let state;

    vm.subscribe(s => { state = s; });

    vm.start();
    await tick();

    fail = true;
    await vm.refresh();

    assert.equal(state.error, "HTTP 500");
    assert.equal(state.events.length, 1);

    vm.stop();
});

test("prevents overlapping event requests", async () => {
    const pending = deferred();
    let calls = 0;

    const api = {
        getEvents: () => {
            calls++;
            return pending.promise;
        }
    };

    const vm = createEventLogViewModel(api);

    vm.start();

    const second = vm.refresh();
    const third = vm.refresh();

    assert.equal(calls, 1);

    pending.resolve({ events: [] });
    await Promise.all([second, third]);
    await tick();

    vm.stop();
});

test("ignores in-flight responses after stop", async () => {
    const pending = deferred();

    const api = {
        getEvents: () => pending.promise
    };

    const vm = createEventLogViewModel(api);
    let state;

    vm.subscribe(s => { state = s; });

    vm.start();
    vm.stop();

    pending.resolve({
        events: [{ sequenceId: 99 }]
    });

    await tick();

    assert.deepEqual(state.events, []);
});

test("does not start duplicate polling timers", async () => {
    let calls = 0;

    const api = {
        getEvents: async () => {
            calls++;
            return { events: [] };
        }
    };

    const vm = createEventLogViewModel(api);

    vm.start();
    vm.start();

    await tick();

    assert.equal(calls, 1);

    vm.stop();
});

test("recovers after a failed event request", async () => {
    let calls = 0;

    const api = {
        getEvents: async () => {
            calls++;

            if (calls === 1) {
                throw new Error("Network unavailable");
            }

            return { events: [{ sequenceId: 2 }] };
        }
    };

    const vm = createEventLogViewModel(api);
    let state;

    vm.subscribe(s => { state = s; });

    vm.start();
    await tick();

    assert.equal(state.error, "Network unavailable");

    await vm.refresh();

    assert.equal(state.error, null);
    assert.equal(state.events.length, 1);

    vm.stop();
});

