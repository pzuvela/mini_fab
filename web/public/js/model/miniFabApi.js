export const miniFabApi = {

	async runWafer() {
		const response = await fetch("/api/run", {
			method: "POST"
		});

		return {
			httpStatus: response.status,
			...await response.json()
		};
	}
};
