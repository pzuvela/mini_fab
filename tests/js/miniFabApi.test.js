import test from "node:test";
import assert from "node:assert/strict";

import { miniFabApi } from "../../web/public/js/model/miniFabApi.js";

test("MiniFab API exposes equipment control operations", () => {
    assert.equal(typeof miniFabApi.runWafer, "function");
    assert.equal(typeof miniFabApi.cancelRun, "function");
    assert.equal(typeof miniFabApi.resetEquipment, "function");
});

test("Run rejection preserves the server's domain error", async () => {
    const previousFetch = globalThis.fetch;
    globalThis.fetch = async () => ({
        status: 409, ok: false,
        async json() { return { accepted: false, error: "Run rejected" }; }
    });
    try {
        const result = await miniFabApi.runWafer(1);
        assert.equal(result.accepted, false);
        assert.equal(result.error, "Run rejected");
    } finally {
        globalThis.fetch = previousFetch;
    }
});

test("Cancel rejects malformed server responses", async () => {
    const previousFetch = globalThis.fetch;
    globalThis.fetch = async () => ({
        status: 502, ok: false,
        async json() { throw new SyntaxError("Invalid JSON"); }
    });
    try {
        await assert.rejects(miniFabApi.cancelRun(), /Invalid API response/);
    } finally {
        globalThis.fetch = previousFetch;
    }
});

test("Reset reports HTTP failures without a domain error", async () => {
    const previousFetch = globalThis.fetch;
    globalThis.fetch = async () => ({
        status: 503, ok: false,
        async json() { return { recovered: false }; }
    });
    try {
        await assert.rejects(miniFabApi.resetEquipment(), /HTTP 503/);
    } finally {
        globalThis.fetch = previousFetch;
    }
});
