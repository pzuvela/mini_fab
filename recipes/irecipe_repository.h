#pragma once

#include <optional>
#include <vector>

#include "domain/recipe.h"

class IRecipeRepository
{
public:
    virtual std::vector<Recipe> getAll() const = 0;

    virtual std::optional<Recipe> getById(int id) const = 0;

    virtual bool insert(const Recipe& recipe) = 0;

    virtual bool update(const Recipe& recipe) = 0;

    virtual bool remove(int id) = 0;

    virtual ~IRecipeRepository() = default;
};

