#pragma once

#include "httplib.h"

class MiniFabController;

namespace MiniFab::Web
{
    void registerRecipeRoutes(
        httplib::Server& server,
        MiniFabController& controller
    );
}

