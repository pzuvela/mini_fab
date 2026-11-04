export const miniFabApi = {

	async runWafer() {
		const response = await fetch("/api/run", {
			method: "POST"
		});

		return {
			httpStatus: response.status,
			...await response.json()
		};
	},

	async getStatus() {
	    const response = await fetch("/api/status", { cache: "no-store" });

	    if (!response.ok) {
	        throw new Error(`HTTP ${response.status}`);
	    }

	    return response.json();
	}
};

