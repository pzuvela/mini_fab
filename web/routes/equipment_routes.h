#pragma once

#include "httplib.h"

class MiniFabController;

namespace MiniFab::Web
{
    void registerEquipmentRoutes(
        httplib::Server& server,
        MiniFabController& controller
    );
}

