#include "web/serialization/recipe_json.h"
#include "web/serialization/json_utils.h"

#include <sstream>

namespace MiniFab::Web
{
    std::string toJson(const Recipe& recipe)
    {
        std::ostringstream out;

        out << "{";
        out << "\"id\":" << recipe.id;
        out << ",\"name\":";

        writeJsonString(out, recipe.name);

        out << ",\"durationSeconds\":"
            << recipe.durationSeconds;

        out << "}";

        return out.str();
    }

    std::string toJson(const std::vector<Recipe>& recipes)
    {
        std::ostringstream out;

        out << "[";

        for (std::size_t i = 0; i < recipes.size(); ++i)
        {
            if (i > 0)
                out << ",";

            out << toJson(recipes[i]);
        }

        out << "]";

        return out.str();
    }
}
