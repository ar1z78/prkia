#ifndef ISETTINGS_H
#define ISETTINGS_H

#include <shared/UnicodeSupport.h>
#include <memory>

namespace aoia 
{
    struct ISettings
    {
        virtual ~ISettings() {}

        /// Gets the value associated with the specified key. Returns empty string if unknown key.
        virtual std::tstring getValue(std::tstring const& key) const = 0;

        /// Assigns a value to the specified key.
        virtual void setValue(std::tstring const& key, std::tstring const& value) = 0;
    };

    typedef std::shared_ptr<ISettings> ISettingsPtr;
}

#endif // ISETTINGS_H
