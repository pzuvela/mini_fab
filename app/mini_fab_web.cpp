#include <iostream>
#include <string>

#include "httplib.h"
#include "controller/mini_fab_controller.h"
#include "web/serialization/equipment_snapshot_json.h"
#include "web/serialization/eqiupment_events_json.h"


int main()
{
	MiniFabController controller;
	httplib::Server server;
	std::string ip = "127.0.0.1";
	int port = 8080;

	if (!server.set_mount_point("/", "./web/public"))
	{
    		std::cerr << "Failed to mount web/public\n";
    	return 1;
	}

	server.Get("/api/health",
		[](const httplib::Request&,
			httplib::Response& response)
		{
			response.set_content(
				R"({"status": "ok", "system": "MiniFab"})",
				"application/json"
			);
		}
	);


	server.Get("/api/status",
		[&controller](const httplib::Request&,
				httplib::Response& response)
		{
			response.set_content(
				MiniFab::Web::toJson(controller.status()),
				"application/json"
			);
		}
	);


	server.Post("/api/run",
    		[&controller](const httplib::Request&,
                  	httplib::Response& response)
    		{
        		OperationResult result = controller.runWafer();

		        if (result == OperationResult::Completed)
        		{
            			response.status = 202; // Accepted
            			response.set_content(
                			R"({"accepted":true})",
                			"application/json"
            			);
        		}
        		else
        		{
            			response.status = 409; // Conflict
            			response.set_content(
                			R"({"accepted":false})",
                			"application/json"
            			);
        		}
    		}
	);

	server.Get("/api/events",
		[&controller](const httplib::Request&,
			httplib::Response& response)
		{
			response.set_content(
				MiniFab::Web::toJson(controller.events()),
				"application/json"
			);
		}
	);

	std::cout << "MiniFab server on port " << port << "\n";

	server.listen(ip, port);
}

