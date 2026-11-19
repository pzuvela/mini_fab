import test from "node:test";
import assert from "node:assert/strict";

import { bindRunControls } from
    "../../web/public/js/view/runControls.js";

function control() {
    const listeners = new Map();

    return {
        addEventListener(type, callback) {
            listeners.set(type, callback);
        },
        trigger(type, value) {
            listeners.get(type)({ target: { value } });
        }
    };
}

test("Wafer dropdown selection is submitted by Run button", async () => {
    const elements = {
        runButton: control(),
        cancelButton: control(),
        resetButton: control(),
        recipeSelect: control(),
        waferSelect: control()
    };

    let selectedWafer = "W1";
    const submitted = [];

    const vm = {
        selectWafer(id) {
            selectedWafer = id;
        },
        selectRecipe() {},
        run() {
            submitted.push(selectedWafer);
        },
        cancelRun() {},
        resetEquipment() {}
    };

    bindRunControls(elements, vm);

    elements.waferSelect.trigger("change", "W2");
    elements.runButton.trigger("click");

    assert.deepEqual(submitted, ["W2"]);
});


