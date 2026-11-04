#include <iostream>
#include <string>

#include "httplib.h"
#include "controller/mini_fab_controller.h"
#include "web/serialization/equipment_snapshot_json.h"


int main()
{
	MiniFabController controller;
	httplib::Server server;
	std::string ip = "127.0.0.1";
	int port = 8080;

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

	std::cout << "MiniFab server on port " << port << "\n";

	server.listen(ip, port);
}

