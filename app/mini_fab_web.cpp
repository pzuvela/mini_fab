#include <iostream>

#include "httplib.h"

#include "controller/mini_fab_controller.h"
#include "recipes/sqlite_recipe_repository.h"
#include "recipes/recipe_manager.h"

#include "web/routes/equipment_routes.h"
#include "web/routes/recipe_routes.h"

int main()
{
    SqliteRecipeRepository repository("minifab.db");
    RecipeManager recipeManager(repository);
    MiniFabController controller(recipeManager);

    httplib::Server server;

    if (!server.set_mount_point("/", "./web/public"))
    {
        std::cerr << "Failed to mount web/public\n";
        return 1;
    }

    MiniFab::Web::registerEquipmentRoutes(server, controller);
    MiniFab::Web::registerRecipeRoutes(server, controller);

    std::cout << "MiniFab server on port 8080\n";

    if (!server.listen("127.0.0.1", 8080))
    {
        std::cerr << "Failed to start MiniFab server\n";
        return 1;
    }

    return 0;
}

