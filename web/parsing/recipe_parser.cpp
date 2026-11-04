#include "web/parsing/recipe_parser.h"

#include <cmath>
#include <stdexcept>
#include <string>

namespace MiniFab::Web
{
    std::optional<Recipe> parseRecipe(
        const httplib::Request& request)
    {
        if (!request.has_param("recipeId") ||
            !request.has_param("recipeName") ||
            !request.has_param("durationSeconds"))
        {
            return std::nullopt;
        }

        try
        {
            const auto idText =
                request.get_param_value("recipeId");

            const auto durationText =
                request.get_param_value("durationSeconds");

            std::size_t idPos = 0;
            std::size_t durationPos = 0;

            Recipe recipe{
                .id = std::stoi(idText, &idPos),
                .name = request.get_param_value("recipeName"),
                .durationSeconds =
                    std::stod(durationText, &durationPos)
            };

            if (idPos != idText.size() ||
                durationPos != durationText.size() ||
                recipe.id <= 0 ||
                recipe.name.empty() ||
                !std::isfinite(recipe.durationSeconds) ||
                recipe.durationSeconds <= 0 ||
                recipe.durationSeconds > 3600)
            {
                return std::nullopt;
            }

            return recipe;
        }
        catch (const std::invalid_argument&)
        {
            return std::nullopt;
        }
        catch (const std::out_of_range&)
        {
            return std::nullopt;
        }
    }
}

