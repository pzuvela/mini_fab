#pragma once

#include "irecipe_repository.h"


class RecipeManager
{
public:
    explicit RecipeManager(IRecipeRepository& repository);
};
