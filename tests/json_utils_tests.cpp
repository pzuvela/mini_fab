
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
