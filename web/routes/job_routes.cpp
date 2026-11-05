#include "web/routes/job_routes.h"
#include "controller/mini_fab_controller.h"
#include "scheduler/job_status.h"

#include <charconv>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>

namespace MiniFab::Web
{
namespace
{
    template<typename T>
    std::optional<T> parsePositiveInteger(std::string_view text)
    {
        T value{};

        const auto [ptr, ec] = std::from_chars(
            text.data(),
            text.data() + text.size(),
            value
        );

        if (ec != std::errc{} ||
            ptr != text.data() + text.size() ||
            value <= 0)
        {
            return std::nullopt;
        }

        return value;
    }

    void sendError(
        httplib::Response& response,
        int status,
        const std::string& message)
    {
        response.status = status;
        response.set_content(
            "{\"error\":\"" + message + "\"}",
            "application/json"
        );
    }
}

void registerJobRoutes(
    httplib::Server& server,
    MiniFabController& controller)
{
    server.Post("/api/jobs",
        [&controller](
            const httplib::Request& request,
            httplib::Response& response)
        {
            if (!request.has_param("waferId") ||
                !request.has_param("recipeId") ||
                !request.has_param("processModuleId"))
            {
                sendError(response, 400, "Missing job parameters");
                return;
            }

            const std::string waferId =
                request.get_param_value("waferId");

            const auto recipeId = parsePositiveInteger<int>(
                request.get_param_value("recipeId")
            );

            const auto pmId = parsePositiveInteger<int>(
                request.get_param_value("processModuleId")
            );

            if (waferId.empty() || !recipeId || !pmId)
            {
                sendError(response, 400, "Invalid job parameters");
                return;
            }

            // Validate before accepting the job.
            // Currently MiniFab supports PM1 and wafer W1.
            if (waferId != "W1" || *pmId != 1)
            {
                sendError(response, 422, "Unsupported equipment target");
                return;
            }

            if (!controller.getRecipe(*recipeId))
            {
                sendError(response, 404, "Recipe not found");
                return;
            }

            const auto jobId = controller.enqueueJob(
                waferId,
                *recipeId,
                *pmId
            );

            response.status = 201;
            response.set_header(
                "Location",
                "/api/jobs/" + std::to_string(jobId)
            );

            response.set_content(
                "{\"jobId\":" + std::to_string(jobId) + "}",
                "application/json"
            );
        }
    );

    server.Get(R"(/api/jobs/([0-9]+))",
        [&controller](
            const httplib::Request& request,
            httplib::Response& response)
        {
            const auto jobId =
                parsePositiveInteger<std::uint64_t>(
                    request.matches[1].str()
                );

            if (!jobId)
            {
                sendError(response, 400, "Invalid job ID");
                return;
            }

            const auto status = controller.jobStatus(*jobId);

            if (!status)
            {
                sendError(response, 404, "Job not found");
                return;
            }

            response.set_content(
                "{\"jobId\":" + std::to_string(*jobId) +
                ",\"status\":\"" + MiniFab::Web::toString(*status) + "\"}",
                "application/json"
            );
        }
    );
}
}
