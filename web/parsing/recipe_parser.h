#pragma once

#include <optional>
#include "httplib.h"
#include "domain/recipe.h"

namespace MiniFab::Web
{
    std::optional<Recipe> parseRecipe(
        const httplib::Request& request
    );
}

