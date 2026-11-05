#pragma onoce

#include "httplib.h"

class MiniFabController;

namespace MiniFab::Web
{
	void registerJobRoutes(httplib::Server& server, MiniFabController& controller);
}

