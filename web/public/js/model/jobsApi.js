async function request(path, options = {}) {
    const response = await fetch(path, {
        cache: "no-store",
        ...options
    });

    let data;

    try {
        data = await response.json();
    } catch {
        throw new Error(`Invalid server response (${response.status})`);
    }

    if (!response.ok) {
        throw new Error(
            data.error ?? `HTTP ${response.status}`
        );
    }

    return data;
}

export const jobsApi = {
    getJobs() {
        return request("/api/jobs");
    },

    getRecipes() { return request("/api/recipes"); },
    getEvents() { return request("/api/events"); },

    getScheduler() {
        return request("/api/scheduler/status");
    },

    startScheduler() {
        return request("/api/scheduler/start", {
            method: "POST"
        });
    },

    stopScheduler() {
        return request("/api/scheduler/stop", {
            method: "POST"
        });
    },

    resetScheduler() {
        return request("/api/scheduler/reset", {
            method: "POST"
        });
    },

    submitJob(waferId, recipeId, processModuleId = 1) {
        const body = new URLSearchParams({
            waferId,
            recipeId: String(recipeId),
            processModuleId: String(processModuleId)
        });

        return request("/api/jobs", {
            method: "POST",
            headers: {
                "Content-Type": "application/x-www-form-urlencoded"
            },
            body
        });
    },

    getWafers()
    {
	return request("/api/wafers");
    },
};

