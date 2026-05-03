#ifndef NAME_HELPER_H
#define NAME_HELPER_H

#include <shared/UnicodeSupport.h>

namespace aoia {
     // Fetches a custom server name from settings based on index.

    std::tstring GetServerName(int serverIndex);
}

#endif
