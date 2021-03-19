#ifndef SASMODELPROVIDER_BASE_NODEBROWSEREXCEPTION_H
#define SASMODELPROVIDER_BASE_NODEBROWSEREXCEPTION_H

#include <common/Exception.h>
#include <string>

namespace SASModelProviderNamespace {

    class NodeBrowserException : public CommonNamespace::Exception {
    public:
        // Copies of the given message and file path are saved internally.
        NodeBrowserException(const std::string& message, const std::string& file,
                int line);

        // interface Exception
        virtual CommonNamespace::Exception* copy() const;
    };

} // namespace SASModelProviderNamespace
#endif /* SASMODELPROVIDER_BASE_NODEBROWSEREXCEPTION_H */
