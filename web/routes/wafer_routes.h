#pragma once

#include "httplib.h"

class MiniFabController;

namespace MiniFab::Web
{
    void registerWaferRoutes(
        httplib::Server& server,
        MiniFabController& controller
    );
}

