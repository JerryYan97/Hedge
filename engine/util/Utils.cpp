#include <iostream>
#include "Utils.h"

#ifdef _WIN32
#include <Windows.h>
#endif

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

    std::string GetExePath()
    {
        std::string res;
#ifdef _WIN32
        char buffer[260];
        DWORD out = GetModuleFileNameA(NULL, buffer, 260);
        res.assign(buffer, out);
#endif
        return res;
    }
}
