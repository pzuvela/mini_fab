#include "sqlite_recipe_repository.h"

#include <cmath>
#include <stdexcept>

namespace
{
    void validateRecipe(const Recipe& recipe)
    {
        if (recipe.id <= 0 ||
            recipe.name.empty() ||
            !std::isfinite(recipe.durationSeconds) ||
            recipe.durationSeconds <= 0 ||
            recipe.durationSeconds > 3600)
        {
            throw std::invalid_argument("Invalid recipe");
        }
    }

    Recipe readRecipe(SQLite::Statement& query)
    {
        return Recipe{
            query.getColumn(0).getInt(),
            query.getColumn(1).getString(),
            query.getColumn(2).getDouble()
        };
    }
}

SqliteRecipeRepository::SqliteRecipeRepository(
    const std::string& databasePath)
    : db_(databasePath,
          SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE)
{
    initializeSchema();
}

void SqliteRecipeRepository::initializeSchema()
{
    db_.exec(R"(
        CREATE TABLE IF NOT EXISTS recipes (
            id INTEGER PRIMARY KEY CHECK(id > 0),
            name TEXT NOT NULL CHECK(length(name) > 0),
            duration_seconds REAL NOT NULL
                CHECK(duration_seconds > 0
                      AND duration_seconds <= 3600)
        );
    )");
}

std::vector<Recipe> SqliteRecipeRepository::getAll() const
{
    std::lock_guard lock(mutex_);

    SQLite::Statement query(
        db_,
        "SELECT id, name, duration_seconds "
        "FROM recipes ORDER BY id"
    );

    std::vector<Recipe> recipes;

    while (query.executeStep())
    {
        recipes.push_back(readRecipe(query));
    }

    return recipes;
}

std::optional<Recipe> SqliteRecipeRepository::getById(
    int id) const
{
    std::lock_guard lock(mutex_);

    SQLite::Statement query(
        db_,
        "SELECT id, name, duration_seconds "
        "FROM recipes WHERE id = ?"
    );

    query.bind(1, id);

    if (!query.executeStep())
        return std::nullopt;

    return readRecipe(query);
}

bool SqliteRecipeRepository::insert(const Recipe& recipe)
{
    validateRecipe(recipe);
    std::lock_guard lock(mutex_);

    SQLite::Statement query(
        db_,
        "INSERT OR IGNORE INTO recipes "
        "(id, name, duration_seconds) "
        "VALUES (?, ?, ?)"
    );

    query.bind(1, recipe.id);
    query.bind(2, recipe.name);
    query.bind(3, recipe.durationSeconds);

    return query.exec() == 1;
}

bool SqliteRecipeRepository::update(const Recipe& recipe)
{
    validateRecipe(recipe);
    std::lock_guard lock(mutex_);

    SQLite::Statement query(
        db_,
        "UPDATE recipes "
        "SET name = ?, duration_seconds = ? "
        "WHERE id = ?"
    );

    query.bind(1, recipe.name);
    query.bind(2, recipe.durationSeconds);
    query.bind(3, recipe.id);

    return query.exec() == 1;
}

bool SqliteRecipeRepository::remove(int id)
{
    std::lock_guard lock(mutex_);

    SQLite::Statement query(
        db_,
        "DELETE FROM recipes WHERE id = ?"
    );

    query.bind(1, id);

    return query.exec() == 1;
}
