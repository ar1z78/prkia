#include "stdafx.h"
#include "PluginManager.h"
#include <memory>

SINGLETON_IMPL(PluginManager);

PluginManager::PluginManager() {}
PluginManager::PluginManager(const PluginManager&) {}
PluginManager::~PluginManager() {}

void PluginManager::AddLibraries(std::tstring const& path)
{
    // Basic path validation using Win32 instead of is_directory
	DWORD dwAttrib = GetFileAttributes(path.c_str());
    if (dwAttrib == INVALID_FILE_ATTRIBUTES || !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY)) {
        return;
    }
	
	// Prepare Win32 search for "*.dll" directly
    std::tstring searchPath = path;
    if (!searchPath.empty() && searchPath.back() != _T('\\') && searchPath.back() != _T('/')) {
        searchPath += _T("\\");
    }
    std::tstring findPath = searchPath + _T("*.dll");

    WIN32_FIND_DATA findData;
    HANDLE hFind = FindFirstFile(findPath.c_str(), &findData);

    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
			// Skip directories (just in case)
            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                continue;
            }
			
			// Construct the full path to the DLL
            std::tstring fullDllPath = searchPath + findData.cFileName;
			
			// Try loading this DLL
            HMODULE hDll = ::LoadLibrary(fullDllPath.c_str());
            if (!hDll) {
                continue;
            }
			// Look for exported plug-in factory function
            FARPROC fp = ::GetProcAddress(hDll, "AOIA_CreatePlugin");
            if (fp) {
                m_factories.push_back(fp);
            }
            else {
                ::FreeLibrary(hDll);
            }
        } while (FindNextFile(hFind, &findData));

        FindClose(hFind);
    }
}

/*
std::vector<std::shared_ptr<aoia::IPluginView>> PluginManager::createPlugins()
{
    std::vector<std::shared_ptr<aoia::IPluginView>> retval;
    for (auto fp : m_factories) {
        typedef aoia::IPluginView* (__cdecl *FactoryFunc)();
        FactoryFunc f = (FactoryFunc)fp;
        
        // Wrap the raw pointer from the factory into a shared_ptr
        std::shared_ptr<aoia::IPluginView> plugin(f());
        if (plugin) {
            retval.push_back(plugin);
        }
    }
    return retval;
}
*/
