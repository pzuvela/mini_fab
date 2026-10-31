#pragma once

#include <stop_token>

#include "domain/wafer.h"
#include "domain/recipe.h"

class IProcessModule
{
public:
	virtual void initialize() = 0;
	virtual bool isReady() const = 0;
	virtual bool process(const Wafer& wafer, const Recipe& recipe, std::stop_token stopToken) = 0;
	virtual ~IProcessModule() = default;
};
