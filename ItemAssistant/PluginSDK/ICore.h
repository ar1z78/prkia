#ifndef ICORE_H
#define ICORE_H

#include <PluginSDK/IContainerManager.h>
#include <PluginSDK/IDBManager.h>
#include <PluginSDK/IGuiServices.h>
#include <memory>

namespace aoia 
{
    struct ICore
    {
		virtual ~ICore() {}
        virtual IGuiServicesPtr GuiServices() const = 0;
        virtual IDBManagerPtr DataServices() const = 0;
        virtual IContainerManagerPtr AccountServices() const = 0;
    };

    typedef std::shared_ptr<ICore> ICorePtr;
}

#endif // ICORE_H
