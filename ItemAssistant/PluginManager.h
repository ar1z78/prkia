#pragma warning(disable: 4251)

#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

//#include <windows.h>
//#include <memory>
#include <ItemAssistantCore.h>
#include <vector>
#include <shared/UnicodeSupport.h>
#include <Shared/Singleton.h>


class ITEMASSISTANTCORE_API PluginManager
{
    SINGLETON(PluginManager);
public:
    ~PluginManager();

    void AddLibraries(std::tstring const& path);
	//std::vector<std::shared_ptr<aoia::IPluginView>> createPlugins();
    //std::vector<std::shared_ptr<PluginViewInterface> > createPlugins();

private:
	std::vector<FARPROC> m_factories;
};

#endif // PLUGINMANAGER_H
