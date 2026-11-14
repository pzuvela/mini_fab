import { createWaferViewModel } from "../viewmodel/waferViewModel.js";
import { createEventLogViewModel } from "../viewmodel/eventLogViewModel.js";
import { renderEventLog } from "../view/eventLogView.js";
import { initializeWafersPage } from "./wafersPage.js";

initializeWafersPage({
    document,
    window,
    waferVm: createWaferViewModel(),
    eventLogVm: createEventLogViewModel(),
    renderEventLog
});
