#include "recipes/recipe_manager.h"

#include <algorithm>
#include <utility>

RecipeManager::RecipeManager(IRecipeRepository& repository)
    : repository_(repository)
{
    for (const auto& recipe : repository_.getAll())
    {
        recipes_.emplace(recipe.id, recipe);
    }
}

std::vector<Recipe> RecipeManager::getAll() const
{
    std::lock_guard lock(mutex_);

    std::vector<Recipe> result;
    result.reserve(recipes_.size());

    for (const auto& [id, recipe] : recipes_)
    {
        result.push_back(recipe);
    }

    std::sort(result.begin(), result.end(),
        [](const Recipe& a, const Recipe& b)
        {
            return a.id < b.id;
        });

    return result;
}

std::optional<Recipe> RecipeManager::getById(int id) const
{
    std::lock_guard lock(mutex_);

    auto it = recipes_.find(id);

    if (it == recipes_.end())
        return std::nullopt;

    return it->second;
}

bool RecipeManager::add(const Recipe& recipe)
{
    std::lock_guard lock(mutex_);

    if (recipes_.contains(recipe.id))
        return false;

    if (!repository_.insert(recipe))
        return false;

    recipes_.insert_or_assign(recipe.id, recipe);
    return true;
}

bool RecipeManager::update(const Recipe& recipe)
{
    std::lock_guard lock(mutex_);

    if (!recipes_.contains(recipe.id))
        return false;

    if (!repository_.update(recipe))
        return false;

    recipes_.insert_or_assign(recipe.id, recipe);
    return true;
}

bool RecipeManager::remove(int id)
{
    std::lock_guard lock(mutex_);

    if (!recipes_.contains(id))
        return false;

    if (!repository_.remove(id))
        return false;

    recipes_.erase(id);
    return true;
}


RecipeUpsertResult RecipeManager::upsert(const Recipe& recipe)
{
    std::lock_guard lock(mutex_);

    const bool exists = recipes_.contains(recipe.id);

    const bool success = exists
        ? repository_.update(recipe)
        : repository_.insert(recipe);

    if (!success)
        return RecipeUpsertResult::Failed;

    recipes_.insert_or_assign(recipe.id, recipe);

    return exists
        ? RecipeUpsertResult::Updated
        : RecipeUpsertResult::Created;
}

