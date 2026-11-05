#include <iostream>
#include <filesystem>

#include "httplib.h"

#include "controller/mini_fab_controller.h"
#include "persistence/sqlite_recipe_repository.h"
#include "recipes/recipe_manager.h"
#include "persistence/sqlite_event_repository.h"
#include "web/routes/equipment_routes.h"
#include "web/routes/recipe_routes.h"
#include "web/routes/job_routes.h"


int main()
{
    SqliteRecipeRepository repository("minifab.db");
    SqliteEventRepository eventRepository("minifab.db");

    RecipeManager recipeManager(repository);
    MiniFabController controller(recipeManager, eventRepository);

    controller.startScheduler();

    httplib::Server server;

    if (!server.set_mount_point("/", MINIFAB_PUBLIC_DIR))
    {
        std::cerr << "Failed to mount " << MINIFAB_PUBLIC_DIR << "\n";
        return 1;
    }

    MiniFab::Web::registerEquipmentRoutes(server, controller);
    MiniFab::Web::registerRecipeRoutes(server, controller);
    MiniFab::Web::registerJobRoutes(server, controller);

    std::cout << "MiniFab server on port 8080\n";

    if (!server.listen("127.0.0.1", 8080))
    {
        std::cerr << "Failed to start MiniFab server\n";
        return 1;
    }

    return 0;
}

