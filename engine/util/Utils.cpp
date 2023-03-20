#include <iostream>
#include "Utils.h"

#ifdef _WIN32
#include <Windows.h>
#include <shobjidl.h>
#include <shlwapi.h>
#endif

namespace Hedge
{
#ifdef _WIN32
    // ================================================================================================================
    std::string SelectFolderDialogWin()
    {
        HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        if (SUCCEEDED(hr))
        {
            // CoCreate the File Open Dialog object.
            IFileDialog* pfd = NULL;
            HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));
            if (SUCCEEDED(hr))
            {

            }
            CoUninitialize();
        }
    }

    // ================================================================================================================
    std::string SelectYmlDialogWin()
    {

    }
#endif

    // ================================================================================================================
    std::string SelectFolderDialog()
    {
        std::string ret;
#ifdef _WIN32
        ret = SelectFolderDialogWin();
#endif
        return ret;
    }

    // ================================================================================================================
    std::string SelectYmlDialog()
    {
        std::string ret;
#ifdef _WIN32
        SelectYmlDialogWin();
#endif
        return ret;
    }

    // ================================================================================================================
    std::string GetFileName(
        const std::string& pathName)
    {
        size_t pos = pathName.rfind("\\");
        return pathName.substr(pos + 1);
    }

    // ================================================================================================================
    std::string GetFileDir(
        const std::string& pathName)
    {
        size_t pos = pathName.rfind("\\");
        return pathName.substr(0, pos);
    }

    // ================================================================================================================
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
