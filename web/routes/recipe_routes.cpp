#include "web/routes/recipe_routes.h"

#include "controller/mini_fab_controller.h"
#include "web/serialization/recipe_json.h"
#include "web/parsing/recipe_parser.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <limits>
#include <optional>


namespace MiniFab::Web
{

namespace
{
    std::optional<int> parseRecipeId(
        const httplib::Request& request)
    {
        try
        {
            const auto idText = request.matches[1].str();

            std::size_t pos = 0;
            int id = std::stoi(idText, &pos);

            if (pos != idText.size() || id <= 0)
                return std::nullopt;

            return id;
        }
        catch (const std::exception&)
        {
            return std::nullopt;
        }
    }

    void sendError(
        httplib::Response& response,
        int status,
        const std::string& message)
    {
        response.status = status;

        // Only fixed, internal error messages are passed here.
        response.set_content(
            "{\"error\":\"" + message + "\"}",
            "application/json"
        );
    }
}

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

server.Get(R"(/api/recipes/([0-9]+))",
    [&controller](const httplib::Request& request,
                  httplib::Response& response)
    {
        auto id = parseRecipeId(request);

        if (!id)
        {
            sendError(response, 400, "Invalid recipe ID");
            return;
        }

        auto recipe = controller.getRecipe(*id);

        if (!recipe)
        {
            sendError(response, 404, "Recipe not found");
            return;
        }

        response.set_content(
            toJson(*recipe),
            "application/json"
        );
    }
);

server.Put(R"(/api/recipes/([0-9]+))",
    [&controller](const httplib::Request& request,
                  httplib::Response& response)
    {
        auto id = parseRecipeId(request);
        auto recipe = parseRecipe(request);

        if (!id || !recipe)
        {
            sendError(response, 400, "Invalid recipe parameters");
            return;
        }

        if (recipe->id != *id)
        {
            sendError(response, 400, "Recipe ID mismatch");
            return;
        }

        try
        {
            auto result = controller.upsertRecipe(*recipe);

            if (result == RecipeUpsertResult::Failed)
            {
                sendError(response, 409, "Recipe write conflict");
                return;
            }

            response.status =
                result == RecipeUpsertResult::Created
                    ? 201
                    : 200;

            if (result == RecipeUpsertResult::Created)
            {
                response.set_header(
                    "Location",
                    "/api/recipes/" + std::to_string(*id)
                );
            }

            response.set_content(
                toJson(*recipe),
                "application/json"
            );
        }
        catch (const std::exception& e)
        {
            std::cerr << "Recipe upsert failed: "
                      << e.what() << '\n';

            sendError(response, 500, "Database operation failed");
        }
    }
);

server.Delete(R"(/api/recipes/([0-9]+))",
    [&controller](const httplib::Request& request,
                  httplib::Response& response)
    {
        auto id = parseRecipeId(request);

        if (!id)
        {
            sendError(response, 400, "Invalid recipe ID");
            return;
        }

        try
        {
            if (!controller.removeRecipe(*id))
            {
                sendError(response, 404, "Recipe not found");
                return;
            }

            response.status = 204;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Recipe delete failed: "
                      << e.what() << '\n';

            sendError(response, 500, "Database operation failed");
        }
    }
);

    }
}

