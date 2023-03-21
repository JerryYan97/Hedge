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
    constexpr COMDLG_FILTERSPEC YamlProjTypes[] = {
        {L"YAML Files (*.yml)",  L"*.yml"},
        {L"YAML Files (*.yaml)", L"*.yaml"}
    };

    // ================================================================================================================
    std::string SaveToFolderDialogWin()
    {
        std::string res;
        HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        if (SUCCEEDED(hr))
        {
            // CoCreate the File Open Dialog object.
            IFileDialog* pfd = NULL;
            HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));
            if (SUCCEEDED(hr))
            {
                // Set the options on the dialog.
                DWORD dwFlags;

                // Before setting, always get the options first in order not to override existing options.
                hr = pfd->GetOptions(&dwFlags);
                if (SUCCEEDED(hr))
                {
                    // In this case, get shell items only for file system items.
                    hr = pfd->SetOptions(dwFlags | FOS_PICKFOLDERS);
                    if (SUCCEEDED(hr))
                    {
                        if (SUCCEEDED(hr))
                        {
                            if (SUCCEEDED(hr))
                            {
                                if (SUCCEEDED(hr))
                                {
                                    // Show the dialog
                                    hr = pfd->Show(NULL);
                                    if (SUCCEEDED(hr))
                                    {
                                        // Obtain the result, once the user clicks the 'Open' button.
                                        // The result is an IShellItem object.
                                        IShellItem* psiResult;
                                        hr = pfd->GetResult(&psiResult);
                                        if (SUCCEEDED(hr))
                                        {
                                            // We are just going to print out the name of the file for sample sake.
                                            PWSTR pszFilePath = NULL;
                                            hr = psiResult->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
                                            std::wstring ws(pszFilePath);
                                            res = std::string(ws.begin(), ws.end());
                                            psiResult->Release();
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            CoUninitialize();
        }
        return res;
    }

    // ================================================================================================================
    std::string SelectYmlDialogWin()
    {
        std::string res;
        HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        if (SUCCEEDED(hr))
        {
            // CoCreate the File Open Dialog object.
            IFileDialog* pfd = NULL;
            HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));
            if (SUCCEEDED(hr))
            {
                // Set the options on the dialog.
                DWORD dwFlags;

                // Before setting, always get the options first in order not to override existing options.
                hr = pfd->GetOptions(&dwFlags);
                if (SUCCEEDED(hr))
                {
                    // In this case, get shell items only for file system items.
                    hr = pfd->SetOptions(dwFlags | FOS_FORCEFILESYSTEM);
                    if (SUCCEEDED(hr))
                    {
                        hr = pfd->SetFileTypes(ARRAYSIZE(YamlProjTypes), YamlProjTypes);
                        if (SUCCEEDED(hr))
                        {
                            // Set the selected file type to the first element in the YamlProjTypes.
                            hr = pfd->SetFileTypeIndex(1);
                            if (SUCCEEDED(hr))
                            {
                                hr = pfd->SetDefaultExtension(L"yml");
                                if (SUCCEEDED(hr))
                                {
                                    // Show the dialog
                                    hr = pfd->Show(NULL);
                                    if (SUCCEEDED(hr))
                                    {
                                        // Obtain the result, once the user clicks the 'Open' button.
                                        // The result is an IShellItem object.
                                        IShellItem* psiResult;
                                        hr = pfd->GetResult(&psiResult);
                                        if (SUCCEEDED(hr))
                                        {
                                            // We are just going to print out the name of the file for sample sake.
                                            PWSTR pszFilePath = NULL;
                                            hr = psiResult->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
                                            std::wstring ws(pszFilePath);
                                            res = std::string(ws.begin(), ws.end());
                                            psiResult->Release();
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            CoUninitialize();
        }
        return res;
    }
#endif

    // ================================================================================================================
    std::string SaveToFolderDialog()
    {
        std::string ret;
#ifdef _WIN32
        ret = SaveToFolderDialogWin();
#endif
        return ret;
    }

    // ================================================================================================================
    std::string SelectYmlDialog()
    {
        std::string ret;
#ifdef _WIN32
        ret = SelectYmlDialogWin();
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
