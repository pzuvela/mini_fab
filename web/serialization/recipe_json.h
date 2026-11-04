#pragma once

#include <string>
#include <vector>
#include "domain/recipe.h"

namespace MiniFab::Web
{
    std::string toJson(const Recipe& recipe);
    std::string toJson(const std::vector<Recipe>& recipes);
}

