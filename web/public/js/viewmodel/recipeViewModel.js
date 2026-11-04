import { miniFabApi } from "../model/miniFabApi.js";

export function createRecipeViewModel(api = miniFabApi) {
    let recipes = [];
    let message = "Loading recipes...";
    let busy = false;

    const subscribers = new Set();

    function getState() {
        return {
            recipes: [...recipes],
            message,
            busy
        };
    }

    function notify() {
        for (const subscriber of subscribers) {
            subscriber(getState());
        }
    }

    function subscribe(callback) {
        subscribers.add(callback);
        callback(getState());

        return () => subscribers.delete(callback);
    }

    async function load() {
        busy = true;
        message = "Loading recipes...";
        notify();

        try {
            recipes = await api.getRecipes();
            message = `${recipes.length} recipes loaded`;
        } catch (error) {
            message = `Load error: ${error.message}`;
        } finally {
            busy = false;
            notify();
        }
    }

    async function save(recipe) {
        busy = true;
        message = "Saving recipe...";
        notify();

        try {
            await api.saveRecipe(recipe);
            recipes = await api.getRecipes();
            message = `Recipe ${recipe.id} saved`;
        } catch (error) {
            message = `Save error: ${error.message}`;
        } finally {
            busy = false;
            notify();
        }
    }

    async function remove(id) {
        busy = true;
        message = `Deleting recipe ${id}...`;
        notify();

        try {
            await api.deleteRecipe(id);
            recipes = await api.getRecipes();
            message = `Recipe ${id} deleted`;
        } catch (error) {
            message = `Delete error: ${error.message}`;
        } finally {
            busy = false;
            notify();
        }
    }

    return {
        subscribe,
        load,
        save,
        remove
    };
}
