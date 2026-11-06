
#include <gtest/gtest.h>

#include <vector>

#include "web/serialization/recipe_json.h"

TEST(RecipeJsonTest, SerializesSingleRecipe)
{
    const Recipe recipe{1, "Etch", 50.0};

    EXPECT_EQ(
        MiniFab::Web::toJson(recipe),
        R"({"id":1,"name":"Etch","durationSeconds":50})"
    );
}

TEST(RecipeJsonTest, SerializesEmptyCollection)
{
    const std::vector<Recipe> recipes;

    EXPECT_EQ(MiniFab::Web::toJson(recipes), "[]");
}

TEST(RecipeJsonTest, SerializesMultipleRecipes)
{
    const std::vector<Recipe> recipes{
        {1, "Etch", 50.0},
        {2, "Clean", 25.0}
    };

    EXPECT_EQ(
        MiniFab::Web::toJson(recipes),
        R"([{"id":1,"name":"Etch","durationSeconds":50},{"id":2,"name":"Clean","durationSeconds":25}])"
    );
}

TEST(RecipeJsonTest, EscapesRecipeName)
{
    const Recipe recipe{3, "Etch \"A\"\nTest", 12.5};

    EXPECT_EQ(
        MiniFab::Web::toJson(recipe),
        R"({"id":3,"name":"Etch \"A\"\nTest","durationSeconds":12.5})"
    );
}

