import { miniFabApi } from "../model/miniFabApi.js";

export async function runWafer() {
	const result = await miniFabApi.runWafer();

	if (result.accepted) {
		return "Wafer run accepted!";
	}

	return `Run rejected (HTTP ${result.httpStatus})`;
}
