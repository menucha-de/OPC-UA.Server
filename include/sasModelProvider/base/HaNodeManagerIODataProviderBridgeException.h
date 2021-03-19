#ifndef SASMODELPROVIDER_BASE_HANODEMANAGERIODATAPROVIDERBRIDGEEXCEPTION_H
#define SASMODELPROVIDER_BASE_HANODEMANAGERIODATAPROVIDERBRIDGEEXCEPTION_H

#include <common/Exception.h>
#include <string>

namespace SASModelProviderNamespace {

    class HaNodeManagerIODataProviderBridgeException : public CommonNamespace::Exception {
    public:
        // Copies of the given message and file path are saved internally.
        HaNodeManagerIODataProviderBridgeException(const std::string& message, const std::string& file,
                int line);

        // interface Exception
        virtual CommonNamespace::Exception* copy() const;
    };

} // namespace SASModelProviderNamespace
#endif /* HaNodeManagerIODataProviderBridgeException */
