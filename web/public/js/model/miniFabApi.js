async function readJson(response) {
    let result;
    try {
        result = await response.json();
    } catch {
        throw new Error(`Invalid API response (HTTP ${response.status})`);
    }
    if (!response.ok && !result.error) {
        throw new Error(`HTTP ${response.status}`);
    }
    return result;
}

export const miniFabApi = {

async runWafer(recipeId) {
    const response = await fetch("/api/run", {
        method: "POST",
        body: new URLSearchParams({
            recipeId: String(recipeId)
        })
    });

    const result = await response.json();

    return {
        httpStatus: response.status,
        ...result
    };
},

	async getStatus() {
	    const response = await fetch("/api/status", { cache: "no-store" });

	    if (!response.ok) {
	        throw new Error(`HTTP ${response.status}`);
	    }

	    return readJson(response);
	},

	async getEvents() {
    		const response = await fetch("/api/events", { cache: "no-store" });

    		if (!response.ok) {
        		throw new Error(`HTTP ${response.status}`);
    		}

    		return readJson(response);
	},

async getRecipes() {
    const response = await fetch("/api/recipes", {
        cache: "no-store"
    });

    if (!response.ok) {
        throw new Error(`HTTP ${response.status}`);
    }

    return readJson(response);
},

async saveRecipe(recipe) {
    const response = await fetch(
        `/api/recipes/${recipe.id}`,
        {
            method: "PUT",
            body: new URLSearchParams({
                recipeId: recipe.id,
                recipeName: recipe.name,
                durationSeconds: recipe.durationSeconds
            })
        }
    );

    const result = await readJson(response);

    if (!response.ok) {
        throw new Error(
            result.error ?? `HTTP ${response.status}`
        );
    }

    return result;
},

async deleteRecipe(id) {
    const response = await fetch(`/api/recipes/${id}`, {
        method: "DELETE"
    });

    if (!response.ok) {
        const result = await readJson(response);
        throw new Error(
            result.error ?? `HTTP ${response.status}`
        );
    }
},

async cancelRun() {
    const response = await fetch("/api/cancel", {
        method: "POST"
    });

    return {
        httpStatus: response.status,
        ...await readJson(response)
    };
},

async resetEquipment() {
    const response = await fetch("/api/reset", {
        method: "POST"
    });

    return {
        httpStatus: response.status,
        ...await readJson(response)
    };
},

};

