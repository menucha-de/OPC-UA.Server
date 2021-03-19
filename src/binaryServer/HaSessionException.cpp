#include "HaSessionException.h"

using namespace CommonNamespace;

HaSessionException::HaSessionException(
        const std::string& message, const std::string& file, int line) :
Exception(message, file, line) {
}

Exception* HaSessionException::copy() const {
    return new HaSessionException(*this);
}
