#include "GeneratorException.h"

using namespace CommonNamespace;

namespace SASModelProviderNamespace {

    GeneratorException::GeneratorException(const std::string& message,
            const std::string& file, int line) :
    Exception(message, file, line) {
    }

    Exception* GeneratorException::copy() const {
        return new GeneratorException(*this);
    }

} // namespace SASModelProviderNamespace
