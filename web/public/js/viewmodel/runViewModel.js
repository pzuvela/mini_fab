import { miniFabApi } from "../model/miniFabApi.js";

export function createRunViewModel(api = miniFabApi) {
	let snapshot = null;
	let events = [];
	let lastEventSequenceId = 0;
	let lastEventRefresh = 0;
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
			message,
			events: events
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

		await refreshEvents();
	}

	async function refreshEvents() {
		const now = Date.now();

		if (now - lastEventRefresh < 1000) return;

		lastEventRefresh = now;

		try {
			const result = await api.getEvents();

			const latestId = result.events.at(-1)?.sequenceId ?? 0;

			if (latestId < lastEventSequenceId) {
				events = [];
				lastEventSequenceId = 0;
			}

			const newEvents = result.events.filter(
				event => event.sequenceId > lastEventSequenceId
			);

			events = [...events, ...newEvents];
			lastEventSequenceId = latestId;

			notify();
		} catch (error) {
			console.error("Event refresh failed:", error);
		}

	}

	async function run(recipe) {
		if (!getState().canRun) return;

		runningCommand = true;
		notify();

		try {
			const result = await api.runWafer(recipe);
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
