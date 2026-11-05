#include <gtest/gtest.h>

#include "controller/mini_fab_controller.h"
#include "persistence/sqlite_recipe_repository.h"
#include "engine/start_result.h"


TEST(MiniFabRecipeControllerTests, ListsSavedRecipes)
{
    SqliteRecipeRepository repository(":memory:");
    RecipeManager manager(repository);

    MiniFabController controller(manager);

    EXPECT_TRUE(controller.addRecipe({1, "Etch", 5.0}));
    EXPECT_TRUE(controller.addRecipe({2, "Deposition", 10.0}));

    auto recipes = controller.getRecipes();

    ASSERT_EQ(recipes.size(), 2);
    EXPECT_EQ(recipes[0].name, "Etch");
    EXPECT_EQ(recipes[1].name, "Deposition");
}

TEST(MiniFabRecipeControllerTests, RetrievesSavedRecipe)
{
    SqliteRecipeRepository repository(":memory:");
    RecipeManager manager(repository);

    MiniFabController controller(manager);

    controller.addRecipe({1, "Etch", 5.0});

    auto recipe = controller.getRecipe(1);

    ASSERT_TRUE(recipe.has_value());
    EXPECT_EQ(recipe->name, "Etch");
}

TEST(MiniFabRecipeControllerTests, RejectsMissingRecipeRun)
{
    SqliteRecipeRepository repository(":memory:");
    RecipeManager manager(repository);

    MiniFabController controller(manager);

    EXPECT_EQ(
        controller.runWafer(999),
        StartResult::Rejected
    );

    EXPECT_EQ(
        controller.status().state,
        EquipmentState::Idle
    );
}

TEST(MiniFabRecipeControllerTests, UpdatesAndDeletesRecipe)
{
    SqliteRecipeRepository repository(":memory:");
    RecipeManager manager(repository);

    MiniFabController controller(manager);

    EXPECT_TRUE(controller.addRecipe({1, "Etch", 5.0}));
    EXPECT_TRUE(controller.updateRecipe({1, "Fast Etch", 2.0}));

    auto recipe = controller.getRecipe(1);

    ASSERT_TRUE(recipe.has_value());
    EXPECT_EQ(recipe->name, "Fast Etch");

    EXPECT_TRUE(controller.removeRecipe(1));
    EXPECT_FALSE(controller.getRecipe(1).has_value());
}
