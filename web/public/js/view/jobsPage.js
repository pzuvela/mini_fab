import { createJobsViewModel } from
    "../viewmodel/jobsViewModel.js";

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
    recipeId: document.querySelector("#recipeId"),
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

    elements.submitJob.disabled = !available;

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
    const waferId = elements.waferId.value;
    const recipeId = Number(elements.recipeId.value);

    if (!Number.isSafeInteger(recipeId) || recipeId <= 0) {
        elements.message.textContent = "Invalid recipe ID";
        return;
    }

    void vm.submitJob(waferId, recipeId);
});

vm.start();

window.addEventListener("pagehide", () => vm.stop());
