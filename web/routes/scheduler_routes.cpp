#include "web/routes/scheduler_routes.h"

#include <sstream>
#include <string>

#include "controller/mini_fab_controller.h"
#include "scheduler/scheduler_state.h"
#include "web/serialization/json_utils.h"

namespace MiniFab::Web
{
namespace
{
    void sendJson(
        httplib::Response& response,
        int status,
        const std::string& body)
    {
        response.status = status;
        response.set_content(body, "application/json");
    }

std::string schedulerJson(MiniFabController& controller)
{
    const auto snapshot = controller.schedulerSnapshot();

    std::ostringstream out;

    out << "{\"state\":\""
        << toString(snapshot.state)
        << "\",\"pendingJobs\":"
        << snapshot.pendingJobs
        << ",\"activeJobId\":";

    if (snapshot.activeJobId)
        out << *snapshot.activeJobId;
    else
        out << "null";

    out << ",\"error\":";

    writeJsonString(out, snapshot.error);

    out << "}";

    return out.str();
}

}

void registerSchedulerRoutes(
    httplib::Server& server,
    MiniFabController& controller)
{
    server.Get("/api/scheduler/status",
        [&controller](
            const httplib::Request&,
            httplib::Response& response)
        {
            sendJson(response, 200, schedulerJson(controller));
        }
    );

    server.Post("/api/scheduler/start",
        [&controller](
            const httplib::Request&,
            httplib::Response& response)
        {
            if (controller.schedulerState() == SchedulerState::Faulted)
            {
                sendJson(response, 409,
                    R"({"error":"Scheduler is faulted"})");
                return;
            }

            controller.startScheduler();

            sendJson(response, 200, schedulerJson(controller));
        }
    );

    server.Post("/api/scheduler/stop",
        [&controller](
            const httplib::Request&,
            httplib::Response& response)
        {
            controller.stopScheduler();

            sendJson(response, 200, schedulerJson(controller));
        }
    );

    server.Post("/api/scheduler/reset",
        [&controller](
            const httplib::Request&,
            httplib::Response& response)
        {
            if (!controller.resetSchedulerFault())
            {
                sendJson(response, 409,
                    R"({"error":"Scheduler fault cannot be reset"})");
                return;
            }

            sendJson(response, 200, schedulerJson(controller));
        }
    );
}
}

