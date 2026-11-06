
#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "web/serialization/wafer_json.h"

using MiniFab::Web::waferToJson;
using MiniFab::Web::wafersToJson;

TEST(WaferJsonTest, SerializesSingleWafer)
{
    const Wafer wafer{
        "W1",
        {Station::LoadPort, 3}
    };

    EXPECT_EQ(
        waferToJson(wafer),
        R"({"id":"W1","location":{"station":"LoadPort","slot":3}})"
    );
}

TEST(WaferJsonTest, SerializesEmptyInventory)
{
    const std::vector<Wafer> wafers;

    EXPECT_EQ(wafersToJson(wafers), R"({"wafers":[]})");
}

TEST(WaferJsonTest, SerializesMultipleWafers)
{
    const std::vector<Wafer> wafers{
        {"W1", {Station::LoadPort, 3}},
        {"W2", {Station::LoadPort, 4}}
    };

    EXPECT_EQ(
        wafersToJson(wafers),
        R"({"wafers":[{"id":"W1","location":{"station":"LoadPort","slot":3}},{"id":"W2","location":{"station":"LoadPort","slot":4}}]})"
    );
}

TEST(WaferJsonTest, EscapesSpecialCharacters)
{
    const Wafer wafer{
        "W\"2\\test\n",
        {Station::LoadPort, 4}
    };

    EXPECT_EQ(
        waferToJson(wafer),
        R"({"id":"W\"2\\test\n","location":{"station":"LoadPort","slot":4}})"
    );
}

TEST(WaferJsonTest, EscapesControlCharacters)
{
    const Wafer wafer{
        std::string("W") + char(0x01),
        {Station::LoadPort, 5}
    };

    EXPECT_EQ(
        waferToJson(wafer),
        R"({"id":"W\u0001","location":{"station":"LoadPort","slot":5}})"
    );
}

TEST(WaferJsonTest, DoesNotModifyOriginalWafers)
{
    const std::vector<Wafer> wafers{
        {"W1", {Station::LoadPort, 3}},
        {"W2", {Station::LoadPort, 4}}
    };

    const auto original = wafers;

    const auto json = wafersToJson(wafers);

    EXPECT_FALSE(json.empty());
    EXPECT_EQ(wafers, original);
}

