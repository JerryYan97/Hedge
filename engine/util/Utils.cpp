#include <iostream>
#include "Utils.h"

namespace Hedge
{
    std::string GetFileName(
        const std::string& pathName)
    {
        size_t pos = pathName.rfind("\\");
        return pathName.substr(pos + 1);
    }

    std::string GetFileDir(
        const std::string& pathName)
    {
        size_t pos = pathName.rfind("\\");
        return pathName.substr(0, pos);
    }
}
