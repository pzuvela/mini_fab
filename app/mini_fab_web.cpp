#include <iostream>
#include <string>
#include "httplib.h"


int main()
{
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

	std::cout << "MiniFab server on port " << port << "\n";

	server.listen(ip, port);
}

