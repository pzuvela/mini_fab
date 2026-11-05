import test from "node:test";
import assert from "node:assert/strict";

import { miniFabApi } from "../../web/public/js/model/miniFabApi.js";

test("MiniFab API exposes equipment control operations", () => {
    assert.equal(typeof miniFabApi.runWafer, "function");
    assert.equal(typeof miniFabApi.cancelRun, "function");
    assert.equal(typeof miniFabApi.resetEquipment, "function");
});
