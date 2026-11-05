#pragma once


enum class EventSeverity{
        Info,
        Warning,
        Error,
        Critical
};



namespace MiniFab
{
    std::string_view toString(EventSeverity severity)
    {
        switch (severity)
        {
        case EventSeverity::Info:     return "Info";
        case EventSeverity::Warning:  return "Warning";
        case EventSeverity::Error:    return "Error";
        case EventSeverity::Critical: return "Critical";
        }

        throw std::invalid_argument("Unknown event severity");
    }

    EventSeverity severityFromString(const std::string& value)
    {
        if (value == "Info") return EventSeverity::Info;
        if (value == "Warning") return EventSeverity::Warning;
        if (value == "Error") return EventSeverity::Error;
        if (value == "Critical") return EventSeverity::Critical;

        throw std::invalid_argument("Unknown event severity");
    }
}

