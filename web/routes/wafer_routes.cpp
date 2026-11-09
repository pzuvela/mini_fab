#include "web/routes/wafer_routes.h"

#include "controller/mini_fab_controller.h"
#include "web/serialization/wafer_json.h"
#include "web/serialization/json_utils.h"
#include "web/parsing/wafer_json_parser.h"
#include "wafer/wafer_registration_result.h"


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


server.Post("/api/wafers",
    [&controller](const httplib::Request& request,
                  httplib::Response& response)
    {
        // Helper: safely serialize a JSON message.
        auto setJsonResponse =
            [&response](int status,
                        std::string_view key,
                        std::string_view message)
        {
            std::ostringstream out;

            out << "{";
            MiniFab::Web::writeJsonString(out, key);
            out << ":";
            MiniFab::Web::writeJsonString(out, message);
            out << "}";

            response.status = status;
            response.set_content(
                out.str(),
                "application/json"
            );
        };

        const auto parsed =
            MiniFab::Web::parseWaferJson(request.body);

        if (!parsed.wafer)
        {
            setJsonResponse(400, "error", parsed.error);
            return;
        }

        const auto result =
            controller.registerWafer(*parsed.wafer);

        switch (result)
        {
            case WaferRegistrationResult::Registered:
                setJsonResponse(
                    201, "status", "registered");
                break;

            case WaferRegistrationResult::InvalidId:
                setJsonResponse(
                    400, "error", "Invalid wafer ID");
                break;

            case WaferRegistrationResult::InvalidLocation:
                setJsonResponse(
                    400, "error", "Invalid wafer location");
                break;

            case WaferRegistrationResult::DuplicateId:
                setJsonResponse(
                    409, "error", "Wafer ID already exists");
                break;

            case WaferRegistrationResult::SlotOccupied:
                setJsonResponse(
                    409, "error", "Slot already occupied");
                break;
        }
    });


    }
}
