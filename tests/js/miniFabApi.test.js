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
        const result = await miniFabApi.runWafer("W2", 1);
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

test("Run API submits selected wafer and recipe", async () => {
    const previousFetch = globalThis.fetch;

    let capturedUrl;
    let capturedOptions;

    globalThis.fetch = async (url, options) => {
        capturedUrl = url;
        capturedOptions = options;

        return {
            status: 202,
            ok: true,
            async json() {
                return { accepted: true, runId: 42 };
            }
        };
    };

    try {
        const result = await miniFabApi.runWafer("W2", 1);

        assert.equal(capturedUrl, "/api/run");
        assert.equal(capturedOptions.method, "POST");

        assert.equal(
            capturedOptions.body.get("waferId"),
            "W2"
        );

        assert.equal(
            capturedOptions.body.get("recipeId"),
            "1"
        );

        assert.equal(result.accepted, true);
        assert.equal(result.runId, 42);
        assert.equal(result.httpStatus, 202);
    } finally {
        globalThis.fetch = previousFetch;
    }
});

test("Wafer API retrieves registered wafers", async () => {
    const previousFetch = globalThis.fetch;

    let capturedUrl;

    globalThis.fetch = async url => {
        capturedUrl = url;

        return {
            ok: true,
            status: 200,
            async json() {
                return {
                    wafers: [
                        {
                            id: "W1",
                            location: {
                                station: "LoadPort",
                                slot: 3
                            }
                        }
                    ]
                };
            }
        };
    };

    try {
        const result = await miniFabApi.getWafers();

        assert.equal(capturedUrl, "/api/wafers");
        assert.equal(result.wafers.length, 1);
        assert.equal(result.wafers[0].id, "W1");
    } finally {
        globalThis.fetch = previousFetch;
    }
});


