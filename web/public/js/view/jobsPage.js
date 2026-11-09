import { createJobsViewModel } from "../viewmodel/jobsViewModel.js";
import { renderEventLog } from "./eventLogView.js";

const vm = createJobsViewModel();

const elements = {
    schedulerState: document.querySelector("#schedulerState"),
    pendingJobs: document.querySelector("#pendingJobs"),
    activeJob: document.querySelector("#activeJob"),
    connection: document.querySelector("#connection"),
    schedulerError: document.querySelector("#schedulerError"),
    startScheduler: document.querySelector("#startScheduler"),
    stopScheduler: document.querySelector("#stopScheduler"),
    resetScheduler: document.querySelector("#resetScheduler"),
    submitJob: document.querySelector("#submitJob"),
    waferId: document.querySelector("#waferId"),
    recipeSelect: document.querySelector("#recipeSelect"),
    eventRows: document.querySelector("#eventRows"),
    eventCount: document.querySelector("#eventCount"),
    eventError: document.querySelector("#eventError"),
    jobRows: document.querySelector("#jobRows"),
    jobCount: document.querySelector("#jobCount"),
    message: document.querySelector("#message")
};

function renderJobs(jobs) {
    const rows = [...jobs].reverse();
    const fragment = document.createDocumentFragment();

    for (const job of rows) {
        const row = document.createElement("tr");

        const values = [
            job.jobId,
            job.waferId,
            job.recipeId,
            job.processModuleId,
            job.status
        ];

        for (const value of values) {
            const cell = document.createElement("td");
            cell.textContent = String(value);

            if (value === job.status) {
                cell.className =
                    `job-status job-${job.status.toLowerCase()}`;
            }

            row.appendChild(cell);
        }

        fragment.appendChild(row);
    }

    elements.jobRows.replaceChildren(fragment);
    elements.jobCount.textContent = `${jobs.length} jobs`;
}

vm.subscribe(state => {
    const scheduler = state.scheduler;
    const schedulerState = scheduler?.state ?? "Unknown";

    elements.schedulerState.textContent = schedulerState;
    elements.schedulerState.className =
        `state-badge state-${schedulerState.toLowerCase()}`;

    elements.pendingJobs.textContent =
        scheduler?.pendingJobs ?? "—";

    elements.activeJob.textContent =
        scheduler?.activeJobId != null
            ? `#${scheduler.activeJobId}`
            : "None";

    elements.connection.textContent =
        state.connected ? "Connected" : "Disconnected";

    elements.schedulerError.textContent =
        scheduler?.error || state.error || "";

    elements.message.textContent = state.message;

    const available = state.connected && !state.busy;

    elements.startScheduler.disabled =
        !available || schedulerState !== "Stopped";

    elements.stopScheduler.disabled =
        !available || schedulerState !== "Running";

    elements.resetScheduler.disabled =
        !available || schedulerState !== "Faulted";

    elements.submitJob.disabled = !available || state.selectedRecipeId === null;

    const select = elements.recipeSelect;
    const expected = state.recipes.map(r => ({
        value: String(r.id), label: `${r.id} — ${r.name}`
    }));
    const changed = select.options.length !== expected.length ||
        expected.some((item, i) => select.options[i].value !== item.value ||
            select.options[i].textContent !== item.label);
    if (changed) {
        select.replaceChildren();
        for (const item of expected) {
            const option = document.createElement("option");
            option.value = item.value;
            option.textContent = item.label;
            select.appendChild(option);
        }
    }
    if (expected.length === 0 && select.options.length === 0) {
        const option = document.createElement("option");
        option.value = "";
        option.textContent = state.recipeError ? "Failed to load recipes" : "No saved recipes";
        select.appendChild(option);
    }
    select.value = state.selectedRecipeId === null ? "" : String(state.selectedRecipeId);
    select.disabled = expected.length === 0;

    elements.eventError.textContent = state.eventError ?? "";

    renderEventLog(state.events, elements);

    renderJobs(state.jobs);
});

elements.startScheduler.addEventListener("click", () => {
    void vm.startScheduler();
});

elements.stopScheduler.addEventListener("click", () => {
    void vm.stopScheduler();
});

elements.resetScheduler.addEventListener("click", () => {
    void vm.resetScheduler();
});

elements.submitJob.addEventListener("click", () => {
    const waferId = elements.waferId.value.trim();
    if (!waferId) {
        elements.message.textContent = "Wafer ID is required";
        return;
    }
    void vm.submitJob(waferId);
});

elements.recipeSelect.addEventListener("change", event => {
    vm.selectRecipe(event.target.value === "" ? null : Number(event.target.value));
});

vm.start();

window.addEventListener("pagehide", () => vm.stop());
