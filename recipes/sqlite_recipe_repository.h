#pragma once

#include <SQLiteCpp/SQLiteCpp.h>
#include <mutex>
#include <string>

#include "recipes/irecipe_repository.h"


class SqliteRecipeRepository : public IRecipeRepository
{
private:
	mutable std::mutex mutex_;
	SQLite::Database db_;

	void initializeSchema();

public:
	explicit SqliteRecipeRepository(const std::string& databasePath);

	std::vector<Recipe> getAll() const override;
	std::optional<Recipe> getById(int id) const override;

	bool insert(const Recipe& recipe) override;
	bool update(const Recipe& recipe) override;
	bool remove(int id) override;
};

