
import test from "node:test";
import assert from "node:assert/strict";

import { createRunViewModel } from
    "../../web/public/js/viewmodel/runViewModel.js";

function createMockApi() {
    let equipmentState = "Idle";
    let events = [];
    const submittedRecipes = [];

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

            return {
                httpStatus: 202,
                accepted: true
            };
        }
    };

    return {
        api,
        submittedRecipes,
        setEquipmentState(value) {
            equipmentState = value;
        },
	addEvent(type) {
	    events.push({
        	sequenceId: events.length + 1,
        	timestamp: Date.now(),
        	type,
        	severity: "Info",
        	message: type
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

    await new Promise(resolve => settimeout(resolve, 1100));
    await vm.refreshEvents();

    assert.equal(vm.getState().runPhase, "finished");
    assert.match(vm.getState().message, /completed successfully/i);
});
