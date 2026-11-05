
import test from "node:test";
import assert from "node:assert/strict";

import { createRunViewModel } from
    "../../web/public/js/viewmodel/runViewModel.js";

function createMockApi() {
    let equipmentState = "Idle";
    let events = [];
    const submittedRecipes = [];
    let nextRunId = 1;
    let currentRunId = null;

    const api = {
        async getStatus() {
            return {
                equipmentState,
                waferLocation: {
                    station: "LoadPort",
                    slot: 3
                },
                robotWaferId: null,
                processModuleWaferId: null
            };
        },

        async getEvents() {
            return { events: [...events] };
        },

        async getRecipes() {
            return [{
                id: 1,
                name: "Test Recipe",
                durationSeconds: 50
            }];
        },

        async runWafer(recipeId) {
            submittedRecipes.push(recipeId);
	    currentRunId = nextRunId++;

            return {
                httpStatus: 202,
                accepted: true,
		runId: currentRunId
            };
        }
    };

    return {
        api,
        submittedRecipes,
        setEquipmentState(value) {
            equipmentState = value;
        },
	addEvent(type, runId = currentRunId) {
	    events.push({
        	sequenceId: events.length + 1,
        	timestamp: Date.now(),
        	type,
        	severity: "Info",
        	message: type,
		runId
    	});
	}
    };
}

async function prepareViewModel(mock) {
    const vm = createRunViewModel(mock.api);

    await vm.loadRecipes();
    await vm.refresh();

    return vm;
}

test("Idle equipment with a recipe can run", async () => {
    const mock = createMockApi();
    const vm = await prepareViewModel(mock);

    assert.equal(vm.getState().equipmentState, "Idle");
    assert.equal(vm.getState().canRun, true);
});

test("Run submits the selected recipe", async () => {
    const mock = createMockApi();
    const vm = await prepareViewModel(mock);

    await vm.run();

    assert.deepEqual(mock.submittedRecipes, [1]);
    assert.match(vm.getState().message, /accepted/i);
});

test("Run button disables during processing", async () => {
    const mock = createMockApi();
    const vm = await prepareViewModel(mock);

    await vm.run();

    mock.setEquipmentState("Processing");
    await vm.refresh();

    assert.equal(vm.getState().canRun, false);
    assert.equal(vm.getState().runPhase, "running");
});

test("Run button re-enables after processing", async () => {
    const mock = createMockApi();
    const vm = await prepareViewModel(mock);

    await vm.run();

    mock.setEquipmentState("Processing");
    await vm.refresh();

    assert.equal(vm.getState().canRun, false);

    mock.setEquipmentState("Idle");
    await vm.refresh();

    assert.equal(vm.getState().canRun, true);
    assert.equal(vm.getState().runPhase, "running");
});

test("Same recipe can be run twice sequentially", async () => {
    const mock = createMockApi();
    const vm = await prepareViewModel(mock);

    await vm.run();

    mock.setEquipmentState("Processing");
    await vm.refresh();

    mock.setEquipmentState("Idle");
    await vm.refresh();

    await vm.run();

    assert.deepEqual(mock.submittedRecipes, [1, 1]);
});

test("Status polling preserves run acceptance message", async () => {
    const mock = createMockApi();
    const vm = await prepareViewModel(mock);

    await vm.run();
    await vm.refresh();

    assert.match(vm.getState().message, /accepted/i);
    assert.notEqual(vm.getState().message, "Connected");
});

test("Status API failure disables Run and shows error", async () => {
    const mock = createMockApi();

    mock.api.getStatus = async () => {
        throw new Error("Connection lost");
    };

    const vm = createRunViewModel(mock.api);

    await vm.loadRecipes();
    await vm.refresh();

    assert.equal(vm.getState().canRun, false);
    assert.match(vm.getState().message, /Connection lost/);
});

test("Rejected run displays rejection", async () => {
    const mock = createMockApi();

    mock.api.runWafer = async () => ({
        httpStatus: 409,
        accepted: false,
        error: "Run rejected"
    });

    const vm = await prepareViewModel(mock);

    await vm.run();

    assert.equal(vm.getState().runPhase, "finished");
    assert.match(vm.getState().message, /rejected/i);
});

test("RunCompleted event finishes the run", async () => {
    const mock = createMockApi();
    const vm = await prepareViewModel(mock);

    await vm.run();

    mock.setEquipmentState("Processing");
    await vm.refresh();

    mock.addEvent("RunCompleted");

    await new Promise(resolve => setTimeout(resolve, 1100));
    await vm.refreshEvents();

    assert.equal(vm.getState().runPhase, "finished");
    assert.match(vm.getState().message, /completed successfully/i);
});

test("RunFailed event finishes the run", async () => {
    const mock = createMockApi();
    const vm = await prepareViewModel(mock);

    await vm.run();

    mock.setEquipmentState("Processing");
    await vm.refresh();

    mock.addEvent("RunFailed");

    await new Promise(resolve => setTimeout(resolve, 1100));
    await vm.refreshEvents();

    assert.equal(vm.getState().runPhase, "finished");
    assert.match(vm.getState().message, /run failed/i);
});

test("RunCancelled event finishes the run", async () => {
    const mock = createMockApi();
    const vm = await prepareViewModel(mock);

    await vm.run();

    mock.setEquipmentState("Processing");
    await vm.refresh();

    mock.addEvent("RunCancelled");

    await new Promise(resolve => setTimeout(resolve, 1100));
    await vm.refreshEvents();

    assert.equal(vm.getState().runPhase, "finished");
    assert.match(vm.getState().message, /run cancelled/i);
});

test("Faulted equipment waits for RunFailed event", async () => {
    const mock = createMockApi();
    const vm = await prepareViewModel(mock);

    await vm.run();

    mock.setEquipmentState("Faulted");
    await vm.refresh();

    assert.equal(vm.getState().runPhase, "accepted");

    mock.addEvent("RunFailed");

    await new Promise(resolve => setTimeout(resolve, 1100));
    await vm.refreshEvents();

    assert.equal(vm.getState().runPhase, "finished");
    assert.match(vm.getState().message, /run failed/i);
});


test("Old RunCompleted event must not finish a newer run", async () => {
    const mock = createMockApi();
    const vm = await prepareViewModel(mock);

    // First run
    await vm.run();

    mock.setEquipmentState("Processing");
    await vm.refresh();

    // First run completes, but event polling hasn't seen it yet.
    mock.setEquipmentState("Idle");
    mock.addEvent("RunCompleted", 1);

    // Equipment is idle, so a second run can be accepted.
    await vm.refresh();
    await vm.run();

    mock.setEquipmentState("Processing");
    await vm.refresh();

    // Delayed event polling retrieves the first run's completion.
    await new Promise(resolve => setTimeout(resolve, 1100));
    await vm.refreshEvents();

    // The old event must not finish the second run.
    assert.equal(vm.getState().runPhase, "running");
});

test("Cancel enabled only while equipment is moving or processing", async () => {
    const mock = createMockApi();
    const vm = await prepareViewModel(mock);

    assert.equal(vm.getState().canCancel, false);

    mock.setEquipmentState("Moving");
    await vm.refresh();
    assert.equal(vm.getState().canCancel, true);

    mock.setEquipmentState("Processing");
    await vm.refresh();
    assert.equal(vm.getState().canCancel, true);

    mock.setEquipmentState("Stopping");
    await vm.refresh();
    assert.equal(vm.getState().canCancel, false);

    mock.setEquipmentState("Idle");
    await vm.refresh();
    assert.equal(vm.getState().canCancel, false);
});

test("Cancel requests cancellation without finishing the run", async () => {
    const mock = createMockApi();
    const vm = await prepareViewModel(mock);

    await vm.run();

    mock.setEquipmentState("Processing");
    await vm.refresh();

    await vm.cancelRun();

    assert.equal(mock.cancelRequests.length, 1);
    assert.equal(vm.getState().runPhase, "running");
    assert.match(vm.getState().message, /cancel/i);
});

test("Cancel rejects when equipment is idle", async () => {
    const mock = createMockApi();
    const vm = await prepareViewModel(mock);

    await vm.cancelRun();

    assert.equal(mock.cancelRequests.length, 0);
});

test("Rejected cancellation displays error", async () => {
    const mock = createMockApi();

    mock.api.cancelRun = async () => ({
        httpStatus: 409,
        accepted: false,
        error: "Cancellation rejected"
    });

    const vm = await prepareViewModel(mock);

    mock.setEquipmentState("Processing");
    await vm.refresh();

    await vm.cancelRun();

    assert.match(vm.getState().message, /rejected/i);
});

test("Reset enabled only when equipment is faulted", async () => {
    const mock = createMockApi();
    const vm = await prepareViewModel(mock);

    assert.equal(vm.getState().canReset, false);

    mock.setEquipmentState("Faulted");
    await vm.refresh();
    assert.equal(vm.getState().canReset, true);

    mock.setEquipmentState("Processing");
    await vm.refresh();
    assert.equal(vm.getState().canReset, false);
});

test("Reset invokes equipment recovery", async () => {
    const mock = createMockApi();
    const vm = await prepareViewModel(mock);

    mock.setEquipmentState("Faulted");
    await vm.refresh();

    await vm.resetEquipment();

    assert.equal(mock.resetRequests.length, 1);
    assert.match(vm.getState().message, /recover|reset/i);
});

test("Failed recovery displays error", async () => {
    const mock = createMockApi();

    mock.api.resetEquipment = async () => ({
        httpStatus: 409,
        recovered: false,
        error: "Equipment recovery failed"
    });

    const vm = await prepareViewModel(mock);

    mock.setEquipmentState("Faulted");
    await vm.refresh();

    await vm.resetEquipment();

    assert.match(vm.getState().message, /failed/i);
});



