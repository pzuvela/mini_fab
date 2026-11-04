#include "web/routes/recipe_routes.h"

#include "controller/mini_fab_controller.h"
#include "web/serialization/recipe_json.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace MiniFab::Web
{
    void registerRecipeRoutes(
        httplib::Server& server,
        MiniFabController& controller)
    {
        server.Get("/api/recipes",
            [&controller](const httplib::Request&,
                          httplib::Response& response)
            {
                response.set_content(
                    toJson(controller.getRecipes()),
                    "application/json"
                );
            }
        );

server.Post("/api/recipes",
    [&controller](const httplib::Request& request,
                  httplib::Response& response)
    {
        auto sendError = [&response](
            int status,
            const std::string& message)
        {
            response.status = status;
            response.set_content(
                "{\"error\":\"" + message + "\"}",
                "application/json"
            );
        };

        if (!request.has_param("recipeId") ||
            !request.has_param("recipeName") ||
            !request.has_param("durationSeconds"))
        {
            sendError(400, "Missing recipe parameters");
            return;
        }

        Recipe recipe;

        try
        {
            const auto idText =
                request.get_param_value("recipeId");

            const auto durationText =
                request.get_param_value("durationSeconds");

            std::size_t idPos = 0;
            std::size_t durationPos = 0;

            recipe.id = std::stoi(idText, &idPos);
            recipe.name =
                request.get_param_value("recipeName");
            recipe.durationSeconds =
                std::stod(durationText, &durationPos);

            if (idPos != idText.size() ||
                durationPos != durationText.size() ||
                recipe.id <= 0 ||
                recipe.name.empty() ||
                !std::isfinite(recipe.durationSeconds) ||
                recipe.durationSeconds <= 0 ||
                recipe.durationSeconds > 3600)
            {
                sendError(400, "Invalid recipe parameters");
                return;
            }
        }
        catch (const std::invalid_argument&)
        {
            sendError(400, "Invalid recipe parameters");
            return;
        }
        catch (const std::out_of_range&)
        {
            sendError(400, "Recipe parameters out of range");
            return;
        }

        try
        {
            if (!controller.addRecipe(recipe))
            {
                sendError(409, "Recipe ID already exists");
                return;
            }

            response.status = 201;
            response.set_header(
                "Location",
                "/api/recipes/" + std::to_string(recipe.id)
            );

            response.set_content(
                MiniFab::Web::toJson(recipe),
                "application/json"
            );
        }
        catch (const std::exception& e)
        {
            std::cerr << "Recipe insert failed: "
                      << e.what() << '\n';

            sendError(500, "Database operation failed");
        }
    }
);

    }
}

