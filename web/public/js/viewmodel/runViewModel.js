import { miniFabApi } from "../model/miniFabApi.js";

export function createRunViewModel(api = miniFabApi) {
	let snapshot = null;
	let message = "Connecting...";
	let runningCommand = false;
	let refreshing = false;
	let timerId = null;

	const subscribers = new Set();

	function getState() {
		return {
			equipmentState: snapshot?.equipmentState ?? "Unknown",
			waferLocation: snapshot?.waferLocation ? `${snapshot.waferLocation.station} / Slot
								${snapshot.waferLocation.slot}`
							      : "Unknown",
			robot: snapshot?.robotWaferId ?? "Empty",
			processModule: snapshot?.processModuleWaferId ?? "Empty",
			canRun: snapshot?.equipmentState === "Idle" && !runningCommand,
			message
		};
	}

	function notify() {
		const state = getState();

		for (const subscriber of subscribers) {
			subscriber(state);
		}
	}

	async function refresh() {
		if (refreshing) return;

		refreshing = true;

		try {
			snapshot = await api.getStatus();
			message = "Connected";
		} catch(error) {
			snapshot = null;
			message = `Status error: ${error.message}`;
		} finally {
			refreshing = false;
			notify();
		}
	}

	async function run() {
		if (!getState().canRun) return;

		runningCommand = true;
		notify();

		try {
			const result = await api.runWafer();
			message = result.accepted ? "Wafer run accepted" : `Run rejected (HTTP ${result.httpStatus})`;
			await refresh();
		} catch (error) {
			message = `Run error ${error.message}`;
		} finally {
			runningCommand = false;
			notify();
		}
	}

	function subscribe(callback) {
		subscribers.add(callback);
		callback(getState());

		return () => subscribers.delete(callback);
	}

	function start() {
		if (timerId !== null) return;

		refresh();
		timerId = setInterval(refresh, 250);
	}

	function stop() {
		clearInterval(timerId);
		timerId = null;
	}

	return {
		getState,
		subscribe,
		refresh,
		run,
		start,
		stop
	};

}
