#include <gtest/gtest.h>

#include "httplib.h"
#include "web/parsing/recipe_parser.h"

namespace
{
    httplib::Request makeRequest(
        const std::string& id,
        const std::string& name,
        const std::string& duration)
    {
        httplib::Request request;
        request.params.emplace("recipeId", id);
        request.params.emplace("recipeName", name);
        request.params.emplace("durationSeconds", duration);
        return request;
    }
}

TEST(RecipeParserTests, ParsesValidRecipe)
{
    auto request = makeRequest("1", "Etch", "5.5");

    auto recipe = MiniFab::Web::parseRecipe(request);

    ASSERT_TRUE(recipe.has_value());
    EXPECT_EQ(recipe->id, 1);
    EXPECT_EQ(recipe->name, "Etch");
    EXPECT_DOUBLE_EQ(recipe->durationSeconds, 5.5);
}

TEST(RecipeParserTests, RejectsMissingParameter)
{
    httplib::Request request;
    request.params.emplace("recipeId", "1");
    request.params.emplace("recipeName", "Etch");

    EXPECT_FALSE(MiniFab::Web::parseRecipe(request).has_value());
}

TEST(RecipeParserTests, RejectsMalformedId)
{
    auto request = makeRequest("12abc", "Etch", "5");

    EXPECT_FALSE(MiniFab::Web::parseRecipe(request).has_value());
}

TEST(RecipeParserTests, RejectsInvalidDuration)
{
    auto request = makeRequest("1", "Etch", "-5");

    EXPECT_FALSE(MiniFab::Web::parseRecipe(request).has_value());
}

TEST(RecipeParserTests, RejectsEmptyName)
{
    auto request = makeRequest("1", "", "5");

    EXPECT_FALSE(MiniFab::Web::parseRecipe(request).has_value());
}

TEST(RecipeParserTests, RejectsNonFiniteDuration)
{
    for (const auto* duration : {"nan", "inf", "-inf"})
    {
        auto request = makeRequest("1", "Etch", duration);

        EXPECT_FALSE(MiniFab::Web::parseRecipe(request).has_value())
            << "Duration: " << duration;
    }
}

TEST(RecipeParserTests, AcceptsMaximumDuration)
{
    auto request = makeRequest("1", "Etch", "3600");

    auto recipe = MiniFab::Web::parseRecipe(request);

    ASSERT_TRUE(recipe.has_value());
    EXPECT_DOUBLE_EQ(recipe->durationSeconds, 3600.0);
}

TEST(RecipeParserTests, RejectsDurationAboveMaximum)
{
    auto request = makeRequest("1", "Etch", "3600.01");

    EXPECT_FALSE(MiniFab::Web::parseRecipe(request).has_value());
}
