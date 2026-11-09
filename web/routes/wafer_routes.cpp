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
        const auto parsed = MiniFab::Web::escapeJson(
            request.body
        );

        if (!parsed.wafer)
        {
            response.status = 400;
	    response.set_content(
    		"{\"error\":\"" + MiniFab::Web::writeJsonString(parsed.error) + "\"}",
    		"application/json"
	    );
            return;
        }

        const auto result = controller.registerWafer(
            *parsed.wafer
        );

	std::ostringstream out;
	out << R"({"error":)";

        switch (result)
        {
            case WaferRegistrationResult::Registered:
                response.status = 201;
		MiniFab::Web:writeJsonstring(out, parsed.error);
		out << "}";
	    	response.set_content(out.str(), "application/json");
                break;

            case WaferRegistrationResult::InvalidId:
            case WaferRegistrationResult::InvalidLocation:
                response.status = 400;
		MiniFab::Web:writeJsonstring(out, parsed.error);
		out << "}";
	    	response.set_content(out.str(), "application/json");
                break;

            case WaferRegistrationResult::DuplicateId:
                response.status = 409;
		MiniFab::Web:writeJsonstring(out, parsed.error);
	    	out << "}";
		response.set_content(out.str(), "application/json");
                break;

            case WaferRegistrationResult::SlotOccupied:
                response.status = 409;
		MiniFab::Web:writeJsonstring(out, parsed.error);
		out << "}";
	    	response.set_content(out.str(), "application/json");
                break;
        }
    });

    }
}
