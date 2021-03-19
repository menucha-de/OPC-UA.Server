#ifndef COMMON_LOGGING_LOGGINGEXCEPTION_H
#define COMMON_LOGGING_LOGGINGEXCEPTION_H

#include <common/Exception.h>
#include <string>

namespace CommonNamespace {

    class LoggingException : public CommonNamespace::Exception {
    public:
        // Copies of the given message and file path are saved internally.
        LoggingException(const std::string& message, const std::string& file,
                int line);

        // interface Exception
        virtual CommonNamespace::Exception* copy() const;
    };

} // CommonNamespace
#endif /* COMMON_LOGGINGEXCEPTION_H */
