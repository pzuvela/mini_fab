
#include <gtest/gtest.h>

#include <string>
#include "web/parsing/wafer_json_parser.h"

using MiniFab::Web::parseWaferJson;

TEST(WaferJsonParserTests, ParsesValidWafer)
{
    auto result = parseWaferJson(
        R"({"id":"W2","station":"LoadPort","slot":4})"
    );

    ASSERT_TRUE(result.wafer.has_value());

    EXPECT_EQ(result.wafer->id, "W2");
    EXPECT_EQ(result.wafer->location.station,
              Station::LoadPort);
    EXPECT_EQ(result.wafer->location.slot, 4);
    EXPECT_TRUE(result.error.empty());
}

TEST(WaferJsonParserTests, AcceptsDifferentFieldOrder)
{
    auto result = parseWaferJson(
        R"({"slot":4,"id":"W2","station":"LoadPort"})"
    );

    ASSERT_TRUE(result.wafer.has_value());
    EXPECT_EQ(result.wafer->id, "W2");
    EXPECT_EQ(result.wafer->location.slot, 4);
}

TEST(WaferJsonParserTests, AcceptsWhitespace)
{
    auto result = parseWaferJson(R"(
    {
        "id": "W2",
        "station": "LoadPort",
        "slot": 4
    }
    )");

    ASSERT_TRUE(result.wafer.has_value());
    EXPECT_EQ(result.wafer->id, "W2");
}

TEST(WaferJsonParserTests, PreservesDomainValidation)
{
    // Parsing succeeds. Registration should reject
    // these values later in WaferManager.
    auto result = parseWaferJson(
        R"({"id":"","station":"LoadPort","slot":0})"
    );

    ASSERT_TRUE(result.wafer.has_value());
    EXPECT_EQ(result.wafer->id, "");
    EXPECT_EQ(result.wafer->location.slot, 0);
}

class InvalidWaferJsonTest
    : public ::testing::TestWithParam<std::string>
{
};

TEST_P(InvalidWaferJsonTest, RejectsInvalidInput)
{
    auto result = parseWaferJson(GetParam());

    EXPECT_FALSE(result.wafer.has_value());
    EXPECT_FALSE(result.error.empty());
}

INSTANTIATE_TEST_SUITE_P(
    InvalidInputs,
    InvalidWaferJsonTest,
    ::testing::Values(
        // Empty or malformed JSON
        "",
        "[]",
        "{}",
        "{",
        R"({"id":"W2")",

        // Missing required fields
        R"({"id":"W2","station":"LoadPort"})",
        R"({"id":"W2","slot":4})",
        R"({"station":"LoadPort","slot":4})",

        // Incorrect types
        R"({"id":123,"station":"LoadPort","slot":4})",
        R"({"id":"W2","station":123,"slot":4})",
        R"({"id":"W2","station":"LoadPort","slot":"4"})",
        R"({"id":"W2","station":"LoadPort","slot":4.5})",
        R"({"id":"W2","station":"LoadPort","slot":true})",

        // Invalid numbers
        R"({"id":"W2","station":"LoadPort","slot":04})",
        R"({"id":"W2","station":"LoadPort","slot":+4})",
        R"({"id":"W2","station":"LoadPort","slot":2147483648})",

        // Duplicate fields
        R"({"id":"W2","id":"W3","station":"LoadPort","slot":4})",
        R"({"id":"W2","station":"LoadPort","slot":4,"slot":5})",

        // Unknown fields or values
        R"({"id":"W2","station":"Robot","slot":4})",
        R"({"id":"W2","station":"LoadPort","slot":4,"foo":1})",

        // Trailing comma - regression test
        R"({"id":"W2","station":"LoadPort","slot":4,})",

        // Leading comma
        R"({,"id":"W2","station":"LoadPort","slot":4})",

        // Double comma
        R"({"id":"W2",,"station":"LoadPort","slot":4})",

        // Trailing content
        R"({"id":"W2","station":"LoadPort","slot":4} garbage)",
        R"({"id":"W2","station":"LoadPort","slot":4}{})"
    )
);


