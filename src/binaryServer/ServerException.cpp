#include "ServerException.h"

using namespace CommonNamespace;

ServerException::ServerException(
        const std::string& message, const std::string& file, int line) :
Exception(message, file, line) {
}

Exception* ServerException::copy() const {
    return new ServerException(*this);
}
