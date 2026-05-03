#ifndef IBACKPACKNAMES_H
#define IBACKPACKNAMES_H

#include <memory>

namespace aoia {

    struct IContainerManager 
    {
        virtual ~IContainerManager() {} // Added virtual destructor for safety
        virtual std::tstring GetContainerName(unsigned int character_id, unsigned int container_id) const = 0;
    };

    // UPDATED: Now uses std::shared_ptr
    typedef std::shared_ptr<IContainerManager> IContainerManagerPtr;

}

#endif // IBACKPACKNAMES_H
