#include <gtest/gtest.h>

#include "recipes/recipe_manager.h"
#include "persistence/sqlite_recipe_repository.h"

TEST(RecipeManagerTests, LoadsRecipesFromRepository)
{
    SqliteRecipeRepository repository(":memory:");

    repository.insert({1, "Etch", 5.0});
    repository.insert({2, "Deposition", 10.0});

    RecipeManager manager(repository);

    auto recipes = manager.getAll();

    ASSERT_EQ(recipes.size(), 2);
    EXPECT_EQ(recipes[0].name, "Etch");
    EXPECT_EQ(recipes[1].name, "Deposition");
}

TEST(RecipeManagerTests, AddUpdatesCacheAndDatabase)
{
    SqliteRecipeRepository repository(":memory:");
    RecipeManager manager(repository);

    EXPECT_TRUE(manager.add({1, "Etch", 5.0}));

    auto cached = manager.getById(1);
    auto stored = repository.getById(1);

    ASSERT_TRUE(cached.has_value());
    ASSERT_TRUE(stored.has_value());

    EXPECT_EQ(cached->name, "Etch");
    EXPECT_EQ(stored->name, "Etch");
}

TEST(RecipeManagerTests, UpdateUpdatesCacheAndDatabase)
{
    SqliteRecipeRepository repository(":memory:");
    RecipeManager manager(repository);

    manager.add({1, "Etch", 5.0});

    EXPECT_TRUE(manager.update({1, "Fast Etch", 2.0}));

    auto cached = manager.getById(1);
    auto stored = repository.getById(1);

    ASSERT_TRUE(cached.has_value());
    ASSERT_TRUE(stored.has_value());

    EXPECT_EQ(cached->name, "Fast Etch");
    EXPECT_EQ(stored->name, "Fast Etch");
    EXPECT_DOUBLE_EQ(stored->durationSeconds, 2.0);
}

TEST(RecipeManagerTests, RemoveUpdatesCacheAndDatabase)
{
    SqliteRecipeRepository repository(":memory:");
    RecipeManager manager(repository);

    manager.add({1, "Etch", 5.0});

    EXPECT_TRUE(manager.remove(1));

    EXPECT_FALSE(manager.getById(1).has_value());
    EXPECT_FALSE(repository.getById(1).has_value());
}

TEST(RecipeManagerTests, RejectsDuplicateId)
{
    SqliteRecipeRepository repository(":memory:");
    RecipeManager manager(repository);

    EXPECT_TRUE(manager.add({1, "Etch", 5.0}));
    EXPECT_FALSE(manager.add({1, "Deposition", 10.0}));

    EXPECT_EQ(manager.getById(1)->name, "Etch");
}

TEST(RecipeManagerTests, MissingRecipeOperationsFail)
{
    SqliteRecipeRepository repository(":memory:");
    RecipeManager manager(repository);

    EXPECT_FALSE(manager.getById(999).has_value());
    EXPECT_FALSE(manager.update({999, "Missing", 5.0}));
    EXPECT_FALSE(manager.remove(999));
}

TEST(RecipeManagerUpsertTests, CreatesNewRecipe)
{
    SqliteRecipeRepository repository(":memory:");
    RecipeManager manager(repository);

    Recipe recipe{92, "Etch", 10.0};

    EXPECT_EQ(
        manager.upsert(recipe),
        RecipeUpsertResult::Created
    );

    auto saved = manager.getById(92);
    ASSERT_TRUE(saved);
    EXPECT_EQ(saved->name, "Etch");
}

TEST(RecipeManagerUpsertTests, UpdatesExistingRecipe)
{
    SqliteRecipeRepository repository(":memory:");
    RecipeManager manager(repository);

    Recipe original{92, "Etch", 10.0};
    Recipe updated{92, "Plasma Etch", 20.0};

    ASSERT_TRUE(manager.add(original));

    EXPECT_EQ(
        manager.upsert(updated),
        RecipeUpsertResult::Updated
    );

    auto saved = manager.getById(92);
    ASSERT_TRUE(saved);
    EXPECT_EQ(saved->name, "Plasma Etch");
    EXPECT_DOUBLE_EQ(saved->durationSeconds, 20.0);
}

TEST(RecipeManagerUpsertTests, RepeatedPutIsIdempotent)
{
    SqliteRecipeRepository repository(":memory:");
    RecipeManager manager(repository);

    Recipe recipe{92, "Etch", 10.0};

    EXPECT_EQ(manager.upsert(recipe), RecipeUpsertResult::Created);
    EXPECT_EQ(manager.upsert(recipe), RecipeUpsertResult::Updated);
    EXPECT_EQ(manager.upsert(recipe), RecipeUpsertResult::Updated);

    EXPECT_EQ(manager.getAll().size(), 1);
}

