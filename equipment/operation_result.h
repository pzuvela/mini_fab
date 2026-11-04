#pragma once

#include <string_view>

enum class OperationResult
{
	Completed,
	Cancelled,
	Failed
};


constexpr std::string_view toString(OperationResult operationResult)
{
	switch (operationResult)
	{
		case OperationResult::Completed: return "Completed";
		case OperationResult::Cancelled: return "Cancelled";
		case OperationResult::Failed: return "Failed";
	}

	return "Unknown";
}
