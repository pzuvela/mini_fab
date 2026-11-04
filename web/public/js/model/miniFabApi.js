export const miniFabApi = {

async runWafer(recipe) {
    const response = await fetch("/api/run", {
        method: "POST",
        body: new URLSearchParams({
            recipeId: recipe.id,
            recipeName: recipe.name,
            durationSeconds: recipe.durationSeconds
        })
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
	},

	async getEvents() {
    		const response = await fetch("/api/events", { cache: "no-store" });

    		if (!response.ok) {
        		throw new Error(`HTTP ${response.status}`);
    		}

    		return response.json();
	}
};

