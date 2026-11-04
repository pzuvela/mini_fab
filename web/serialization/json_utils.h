#pragma once

#include <ostream>
#include <string_view>

namespace MiniFab::Web
{
    void writeJsonString(
        std::ostream& out,
        std::string_view value
    );
}

