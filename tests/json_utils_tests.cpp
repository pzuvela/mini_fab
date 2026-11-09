
#include <gtest/gtest.h>

#include <sstream>
#include <string>

#include "web/serialization/json_utils.h"

TEST(JsonUtilsTest, WritesOrdinaryString)
{
    std::ostringstream out;

    MiniFab::Web::writeJsonString(out, "MiniFab");

    EXPECT_EQ(out.str(), R"("MiniFab")");
}

TEST(JsonUtilsTest, EscapesSpecialCharacters)
{
    std::ostringstream out;

    MiniFab::Web::writeJsonString(
        out, "quote\" slash\\ newline\n tab\t"
    );

    EXPECT_EQ(
        out.str(),
        R"("quote\" slash\\ newline\n tab\t")"
    );
}

TEST(JsonUtilsTest, EscapesControlCharacters)
{
    std::ostringstream out;

    const std::string input =
        std::string("A") + char(0x01) + char(0x1F);

    MiniFab::Web::writeJsonString(out, input);

    EXPECT_EQ(out.str(), R"("A\u0001\u001f")");
}

TEST(JsonUtilsTest, PreservesUtf8)
{
    std::ostringstream out;

    MiniFab::Web::writeJsonString(out, "Café");

    EXPECT_EQ(out.str(), R"("Café")");
}

TEST(JsonUtilsTest, HandlesEmptyString)
{
    std::ostringstream out;

    MiniFab::Web::writeJsonString(out, "");

    EXPECT_EQ(out.str(), R"("")");
}

TEST(JsonUtilsTests, ParseSimpleString)
{
    std::string_view input = R"("W2")";
    std::size_t pos = 0;

    auto result = MiniFab::Web::parseJsonString(input, pos);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "W2");
    EXPECT_EQ(pos, input.size());
}

TEST(JsonUtilsTests, ParseEscapedString)
{
    std::string_view input = R"("Hello\nWorld")";
    std::size_t pos = 0;

    auto result = MiniFab::Web::parseJsonString(input, pos);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "Hello\nWorld");
}

TEST(JsonUtilsTests, RejectUnterminatedString)
{
    std::string_view input = "\"W2";
    std::size_t pos = 0;

    EXPECT_FALSE(MiniFab::Web::parseJsonString(input, pos).has_value());
}

TEST(JsonUtilsTests, RejectInvalidEscape)
{
    std::string_view input = R"("W\q2")";
    std::size_t pos = 0;

    EXPECT_FALSE(MiniFab::Web::parseJsonString(input, pos).has_value());
}

