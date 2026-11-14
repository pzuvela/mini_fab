// Page wiring extracted from wafers.html so the real DOM codepath can be tested.
export function initializeWafersPage({ document, window, waferVm, eventLogVm, renderEventLog }) {
    const vm = waferVm;
    
        const form = document.querySelector("#waferForm");
        const emptyInventory = document.querySelector("#emptyInventory");
        const idInput = document.querySelector("#waferId");
        const slotInput = document.querySelector("#waferSlot");
        const registerButton = document.querySelector("#registerButton");
    
        const message = document.querySelector("#message");
        const rows = document.querySelector("#waferRows");
        const count = document.querySelector("#waferCount");
    
    const recoveryForm =
        document.querySelector("#recoveryForm");
    
    const recoveryWaferId =
        document.querySelector("#recoveryWaferId");
    
    const recoverySlot =
        document.querySelector("#recoverySlot");
    
    const recoveryButton =
        document.querySelector("#recoveryButton");
    
        vm.subscribe(state => {
            message.textContent = state.message;
    
            count.textContent =
                `${state.wafers.length} wafers`;
    
            registerButton.disabled = state.busy;
            idInput.disabled = state.busy;
            slotInput.disabled = state.busy;
    
    recoveryButton.disabled = state.busy;
    recoverySlot.disabled = state.busy;
    
    	emptyInventory.hidden = !state.loaded || state.wafers.length !== 0;
    
            rows.replaceChildren();
    
    
    for (const wafer of state.wafers) {
        const row = document.createElement("tr");
    
        for (const value of [
            wafer.id,
            wafer.location.station,
            wafer.location.slot
        ]) {
            const cell = document.createElement("td");
            cell.textContent = value;
            row.appendChild(cell);
        }
    
        const actionCell = document.createElement("td");
    
        if (wafer.location.station === "ProcessModule") {
            const button = document.createElement("button");
    
            button.type = "button";
            button.textContent = "Recover";
            button.disabled = state.busy;
    
            button.addEventListener("click", () => {
                recoveryWaferId.value = wafer.id;
                recoverySlot.focus();
            });
    
            actionCell.appendChild(button);
        } else {
            actionCell.textContent = "—";
        }
    
        row.appendChild(actionCell);
        rows.appendChild(row);
    }
    
        });
    
        form.addEventListener("submit", async event => {
            event.preventDefault();
    
            if (!form.reportValidity()) return;
    
            const id = idInput.value.trim();
            const slot = Number(slotInput.value);
    
            await vm.register(id, slot);
        });
    
    const eventVm = eventLogVm;
    
    const eventElements = {
        eventRows: document.querySelector("#eventRows"),
        eventCount: document.querySelector("#eventCount"),
        eventError: document.querySelector("#eventError")
    };
    
    // Inventory loading must not depend on event-log initialization.
    vm.load();
    
    eventVm.subscribe(state => {
        renderEventLog(state.events, eventElements);
        eventElements.eventError.textContent =
            state.error ?? "";
    });
    
    recoveryForm.addEventListener("submit", async event => {
        event.preventDefault();
    
        if (!recoveryForm.reportValidity()) return;
    
        const waferId = recoveryWaferId.value.trim();
        const slot = Number(recoverySlot.value);
    
        await vm.recover(waferId, slot);
    });
    
    eventVm.start();
    
    window.addEventListener("pagehide", () => {
        eventVm.stop();
    });
}
