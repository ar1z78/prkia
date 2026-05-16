#include "StdAfx.h"
#include "namehelper.h"
#include "AOManager.h"
#include "settings/ISettings.h"
#include <sstream>

namespace aoia {

    std::tstring GetServerName(int serverIndex)
    {





        // Construct keys like itemurlname1, itemurlname2...
        std::tstringstream ss;
        ss << _T("itemurlname") << serverIndex;
        std::tstring key = ss.str();
        
        std::tstring defaultName;
        if (serverIndex == 1)      defaultName = _T("Auno");
        else if (serverIndex == 2) defaultName = _T("AO Galaxy");
        else                       defaultName = _T("TinkerItems");

        aoia::ISettingsPtr settings = AOManager::instance().getSettings();
        
        if (!settings) return defaultName;

        std::tstring name = settings->getValue(key);

        // Auto-populate the .conf file if it's the first time
        if (name.empty()) {
            name = defaultName;
            settings->setValue(key, name);
        }
        return name;
    }
}
