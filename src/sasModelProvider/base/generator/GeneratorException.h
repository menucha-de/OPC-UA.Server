#ifndef SASMODELPROVIDER_BASE_GENERATOR_DATAGENERATOREXCEPTION_H
#define SASMODELPROVIDER_BASE_GENERATOR_DATAGENERATOREXCEPTION_H

#include <common/Exception.h>
#include <string>

namespace SASModelProviderNamespace {

    class GeneratorException : public CommonNamespace::Exception {
    public:
        // Copies of the given message and file path are saved internally.
        GeneratorException(const std::string& message, const std::string& file,
                int line);

        // interface Exception
        virtual CommonNamespace::Exception* copy() const;
    };

} // namespace SASModelProviderNamespace
#endif /* SASMODELPROVIDER_BASE_GENERATOR_DATAGENERATOREXCEPTION_H */
