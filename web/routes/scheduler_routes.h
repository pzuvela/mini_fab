#pragma once

#include "httplib.h"

class MiniFabController;

namespace MiniFab::Web
{
    void registerSchedulerRoutes(
        httplib::Server& server,
        MiniFabController& controller
    );
}

