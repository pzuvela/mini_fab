
#include <gtest/gtest.h>
#include <filesystem>
#include <stdexcept>

#include "persistence/sqlite_wafer_repository.h"

TEST(SqliteWaferRepositoryTests, InsertAndRetrieve)
{
    SqliteWaferRepository repo(":memory:");

    EXPECT_TRUE(repo.insert({
        "W2", {Station::LoadPort, 4}
    }));

    const auto wafers = repo.getAll();

    ASSERT_EQ(wafers.size(), 1);
    EXPECT_EQ(wafers[0].id, "W2");
    EXPECT_EQ(wafers[0].location.station, Station::LoadPort);
    EXPECT_EQ(wafers[0].location.slot, 4);
}

TEST(SqliteWaferRepositoryTests, DuplicateIdRejected)
{
    SqliteWaferRepository repo(":memory:");

    EXPECT_TRUE(repo.insert({
        "W2", {Station::LoadPort, 4}
    }));

    EXPECT_FALSE(repo.insert({
        "W2", {Station::LoadPort, 5}
    }));

    EXPECT_EQ(repo.getAll().size(), 1);
}

TEST(SqliteWaferRepositoryTests, OccupiedSlotRejected)
{
    SqliteWaferRepository repo(":memory:");

    EXPECT_TRUE(repo.insert({
        "W2", {Station::LoadPort, 4}
    }));

    EXPECT_FALSE(repo.insert({
        "W3", {Station::LoadPort, 4}
    }));

    EXPECT_EQ(repo.getAll().size(), 1);
}

TEST(SqliteWaferRepositoryTests, LocationUpdatePersists)
{
    SqliteWaferRepository repo(":memory:");

    ASSERT_TRUE(repo.insert({
        "W2", {Station::LoadPort, 4}
    }));

    EXPECT_TRUE(repo.updateLocation(
        "W2", {Station::ProcessModule, 1}
    ));

    const auto wafers = repo.getAll();

    ASSERT_EQ(wafers.size(), 1);
    EXPECT_EQ(wafers[0].location.station,
              Station::ProcessModule);
    EXPECT_EQ(wafers[0].location.slot, 1);
}

TEST(SqliteWaferRepositoryTests, InvalidWaferRejected)
{
    SqliteWaferRepository repo(":memory:");

    EXPECT_THROW(
        repo.insert({"", {Station::LoadPort, 4}}),
        std::invalid_argument
    );

    EXPECT_THROW(
        repo.insert({"W2", {Station::LoadPort, 0}}),
        std::invalid_argument
    );
}

TEST(SqliteWaferRepositoryTests, SurvivesRestart)
{
    namespace fs = std::filesystem;

    const auto path = fs::temp_directory_path() /
        "minifab_wafer_persistence_test.sqlite";

    fs::remove(path);

    try
    {
        {
            SqliteWaferRepository repo(path.string());

            ASSERT_TRUE(repo.insert({
                "W2", {Station::LoadPort, 4}
            }));
        }

        {
            SqliteWaferRepository repo(path.string());

            const auto wafers = repo.getAll();

            ASSERT_EQ(wafers.size(), 1);
            EXPECT_EQ(wafers[0].id, "W2");
            EXPECT_EQ(wafers[0].location.slot, 4);
        }
    }
    catch (...)
    {
        fs::remove(path);
        throw;
    }

    fs::remove(path);
}
