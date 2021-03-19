#ifndef SASMODELPROVIDER_BASE_CONVERSIONEXCEPTION_H
#define SASMODELPROVIDER_BASE_CONVERSIONEXCEPTION_H

#include <common/Exception.h>
#include <string>

namespace SASModelProviderNamespace {

    class ConversionException : public CommonNamespace::Exception {
    public:
        // Copies of the given message and file path are saved internally.
        ConversionException(const std::string& message, const std::string& file,
                int line);

        // interface Exception
        virtual CommonNamespace::Exception* copy() const;
    };

} // namespace SASModelProviderNamespace
#endif /* SASMODELPROVIDER_BASE_CONVERSIONEXCEPTION_H */
