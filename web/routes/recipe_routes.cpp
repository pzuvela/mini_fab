#include "web/routes/recipe_routes.h"

#include "controller/mini_fab_controller.h"
#include "web/serialization/recipe_json.h"
#include "web/parsing/recipe_parser.h"

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
        auto recipe = parseRecipe(request);

        if (!recipe)
        {
            response.status = 400;
            response.set_content(
                R"({"error":"Invalid recipe parameters"})",
                "application/json"
            );
            return;
        }

        try
        {
            if (!controller.addRecipe(*recipe))
            {
                response.status = 409;
                response.set_content(
                    R"({"error":"Recipe ID already exists"})",
                    "application/json"
                );
                return;
            }

            response.status = 201;
            response.set_header(
                "Location",
                "/api/recipes/" + std::to_string(recipe->id)
            );

            response.set_content(
                toJson(*recipe),
                "application/json"
            );
        }
        catch (const std::exception& e)
        {
            std::cerr << "Recipe insert failed: "
                      << e.what() << '\n';

            response.status = 500;
            response.set_content(
                R"({"error":"Database operation failed"})",
                "application/json"
            );
        }
    }
);
    }
}

