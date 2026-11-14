import test from "node:test";
import assert from "node:assert/strict";
import { initializeWafersPage } from "../../web/public/js/pages/wafersPage.js";

class Element {
    constructor() { this.children = []; this.listeners = new Map(); this.textContent = ""; this.value = ""; this.disabled = false; this.hidden = false; this.focused = false; }
    addEventListener(type, listener) { this.listeners.set(type, listener); }
    appendChild(child) { this.children.push(child); }
    replaceChildren(...children) { this.children = children; }
    focus() { this.focused = true; }
    reportValidity() { return true; }
    async dispatch(type) { const listener = this.listeners.get(type); assert.ok(listener, `Missing ${type} handler`); return listener({ preventDefault() {} }); }
}

function setup({ wafers = [], eventSubscribeThrows = false } = {}) {
    const elements = new Map();
    const document = {
        querySelector(selector) { if (!elements.has(selector)) elements.set(selector, new Element()); return elements.get(selector); },
        createElement() { return new Element(); }
    };
    const calls = [];
    const waferVm = {
        subscribe(listener) { this.listener = listener; listener({ wafers: [], loaded: false, busy: false, message: "Loading wafers..." }); },
        load() { calls.push("load"); this.listener({ wafers, loaded: true, busy: false, message: "" }); return Promise.resolve(); },
        recover(id, slot) { calls.push(["recover", id, slot]); return Promise.resolve(); },
        register(id, slot) { calls.push(["register", id, slot]); return Promise.resolve(); }
    };
    const eventLogVm = {
        subscribe(listener) { calls.push("eventSubscribe"); if (eventSubscribeThrows) throw Error("Event log unavailable"); listener({ events: [], error: null }); },
        start() { calls.push("eventStart"); },
        stop() { calls.push("eventStop"); }
    };
    const window = { addEventListener(type, listener) { this[type] = listener; } };
    const init = () => initializeWafersPage({ document, window, waferVm, eventLogVm, renderEventLog() {} });
    return { elements, calls, init, window, get: selector => document.querySelector(selector) };
}

const wafers = [
    { id: "W1", location: { station: "LoadPort", slot: 3 } },
    { id: "W2", location: { station: "ProcessModule", slot: 1 } }
];

test("Wafers page initializes and loads inventory", () => {
    const ctx = setup({ wafers });
    ctx.init();
    assert.equal(ctx.get("#message").textContent, "");
    assert.equal(ctx.get("#waferCount").textContent, "2 wafers");
    assert.equal(ctx.get("#waferRows").children.length, 2);
    assert.equal(ctx.calls[0], "load");
});

test("Recovery action selects ProcessModule wafer", async () => {
    const ctx = setup({ wafers });
    ctx.init();
    const rows = ctx.get("#waferRows").children;
    assert.equal(rows[0].children[3].textContent, "—");
    await rows[1].children[3].children[0].dispatch("click");
    assert.equal(ctx.get("#recoveryWaferId").value, "W2");
    assert.equal(ctx.get("#recoverySlot").focused, true);
});

test("Recovery form submits selected wafer and destination", async () => {
    const ctx = setup({ wafers });
    ctx.init();
    await ctx.get("#waferRows").children[1].children[3].children[0].dispatch("click");
    ctx.get("#recoverySlot").value = "4";
    await ctx.get("#recoveryForm").dispatch("submit");
    assert.deepEqual(ctx.calls.find(call => Array.isArray(call)), ["recover", "W2", 4]);
});

test("Wafer loading begins even if event-log subscription throws", () => {
    const ctx = setup({ wafers, eventSubscribeThrows: true });
    assert.throws(ctx.init, /Event log unavailable/);
    assert.equal(ctx.get("#waferRows").children.length, 2);
    assert.equal(ctx.calls[0], "load");
});

test("Pagehide stops event-log polling", () => {
    const ctx = setup({ wafers });
    ctx.init();
    ctx.window.pagehide();
    assert.ok(ctx.calls.includes("eventStop"));
});
