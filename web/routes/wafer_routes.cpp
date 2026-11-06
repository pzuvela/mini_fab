#include "web/routes/wafer_routes.h"

#include "controller/mini_fab_controller.h"
#include "web/serialization/wafer_json.h"

namespace MiniFab::Web
{
    void registerWaferRoutes(
        httplib::Server& server,
        MiniFabController& controller)
    {
        server.Get("/api/wafers",
            [&controller](const httplib::Request&,
                          httplib::Response& response)
            {
                const auto wafers = controller.wafersSnapshot();

                response.set_content(
                    wafersToJson(wafers),
                    "application/json"
                );
            }
        );
    }
}
