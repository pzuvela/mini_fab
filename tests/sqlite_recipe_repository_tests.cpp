#include <gtest/gtest.h>

#include <filesystem>
#include <stdexcept>

#include "recipes/sqlite_recipe_repository.h"

TEST(SqliteRecipeRepositoryTests, InsertAndRetrieve)
{
    SqliteRecipeRepository repo(":memory:");

    Recipe recipe{1, "Etch", 5.0};

    EXPECT_TRUE(repo.insert(recipe));

    auto result = repo.getById(1);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->id, 1);
    EXPECT_EQ(result->name, "Etch");
    EXPECT_DOUBLE_EQ(result->durationSeconds, 5.0);
}

TEST(SqliteRecipeRepositoryTests, DuplicateIdRejected)
{
    SqliteRecipeRepository repo(":memory:");

    EXPECT_TRUE(repo.insert({1, "Etch", 5.0}));
    EXPECT_FALSE(repo.insert({1, "Deposition", 10.0}));

    EXPECT_EQ(repo.getById(1)->name, "Etch");
}

TEST(SqliteRecipeRepositoryTests, UpdateRecipe)
{
    SqliteRecipeRepository repo(":memory:");

    repo.insert({1, "Etch", 5.0});

    EXPECT_TRUE(repo.update({1, "Fast Etch", 2.5}));

    auto result = repo.getById(1);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->name, "Fast Etch");
    EXPECT_DOUBLE_EQ(result->durationSeconds, 2.5);
}

TEST(SqliteRecipeRepositoryTests, DeleteRecipe)
{
    SqliteRecipeRepository repo(":memory:");

    repo.insert({1, "Etch", 5.0});

    EXPECT_TRUE(repo.remove(1));
    EXPECT_FALSE(repo.getById(1).has_value());
    EXPECT_FALSE(repo.remove(1));
}

TEST(SqliteRecipeRepositoryTests, GetAllRecipes)
{
    SqliteRecipeRepository repo(":memory:");

    repo.insert({2, "Deposition", 10.0});
    repo.insert({1, "Etch", 5.0});

    auto recipes = repo.getAll();

    ASSERT_EQ(recipes.size(), 2);
    EXPECT_EQ(recipes[0].id, 1);
    EXPECT_EQ(recipes[1].id, 2);
}

TEST(SqliteRecipeRepositoryTests, InvalidRecipeRejected)
{
    SqliteRecipeRepository repo(":memory:");

    EXPECT_THROW(
        repo.insert({0, "Etch", 5.0}),
        std::invalid_argument
    );

    EXPECT_THROW(
        repo.insert({1, "", 5.0}),
        std::invalid_argument
    );

    EXPECT_THROW(
        repo.insert({1, "Etch", -5.0}),
        std::invalid_argument
    );

    EXPECT_THROW(
        repo.insert({1, "Etch", 3601.0}),
        std::invalid_argument
    );
}

TEST(SqliteRecipeRepositoryTests, MissingRecipe)
{
    SqliteRecipeRepository repo(":memory:");

    EXPECT_FALSE(repo.getById(999).has_value());
    EXPECT_FALSE(repo.update({999, "Missing", 5.0}));
}

TEST(SqliteRecipeRepositoryTests, PersistsAcrossConnections)
{
    namespace fs = std::filesystem;

    const auto path =
        fs::temp_directory_path() /
        "minifab_recipe_persistence_test.sqlite";

    fs::remove(path);

    try
    {
        {
            SqliteRecipeRepository repo(path.string());

            EXPECT_TRUE(
                repo.insert({42, "Persistent Etch", 15.0})
            );
        }

        {
            SqliteRecipeRepository repo(path.string());

            auto recipe = repo.getById(42);

            ASSERT_TRUE(recipe.has_value());
            EXPECT_EQ(recipe->name, "Persistent Etch");
            EXPECT_DOUBLE_EQ(recipe->durationSeconds, 15.0);
        }
    }
    catch (...)
    {
        fs::remove(path);
        throw;
    }

    fs::remove(path);
}
