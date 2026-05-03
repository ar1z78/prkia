#ifndef PLUGINVIEWINTERFACE_H
#define PLUGINVIEWINTERFACE_H

#include <shared/Signals.h> 
#include <Parsers/AOMessageBase.h>
#include <Parsers/AOClientMessageBase.h>
#include <memory>
#include <string>

namespace aoia
{
    struct IPluginView
    {
		virtual ~IPluginView() {}
		
        typedef aoia::Signal<void ()>  status_signal_t;
        typedef aoia::SignalConnection connection_t;

        virtual connection_t connectStatusChanged(status_signal_t::slot_function_type slot) = 0;
        virtual void disconnect(connection_t slot) = 0;

        virtual void OnAOServerMessage(Parsers::AOMessageBase &msg) = 0;
        virtual void OnAOClientMessage(Parsers::AOClientMessageBase &msg) = 0;
        virtual bool PreTranslateMsg(MSG* pMsg) = 0;
        virtual HWND GetWindow() const = 0;
        virtual void OnActive(bool doActivation) = 0;
        virtual HWND GetToolbar() const = 0;
        virtual std::tstring GetStatusText() const = 0;
    };

    typedef std::shared_ptr<IPluginView> IPluginViewPtr;
}

/// Signature of plugin DLL factory function.
typedef aoia::IPluginViewPtr (*AOIA_CreatePlugin)(std::string const&);

#endif // PLUGINVIEWINTERFACE_H
