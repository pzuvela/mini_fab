#pragma once

#include <unordered_map>
#include <mutex>
#include <optional>
#include <vector>
#include "irecipe_repository.h"


class RecipeManager
{
private:
	IRecipeRepository& repository_;
	mutable std::mutex mutex_;
	std::unordered_map<int, Recipe> recipes_;

public:
	explicit RecipeManager(IRecipeRepository& repository);

	std::vector<Recipe> getAll() const;
	std::optional<Recipe> getById(int id) const;

	bool add(const Recipe& recipe);
	bool update(const Recipe& recipe);
	bool remove(int id);
};
