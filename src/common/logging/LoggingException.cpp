#include <common/logging/LoggingException.h>

namespace CommonNamespace {

    LoggingException::LoggingException(const std::string& message,
            const std::string& file, int line) :
    Exception(message, file, line) {
    }

    Exception* LoggingException::copy() const {
        return new LoggingException(*this);
    }

} // CommonNamespace