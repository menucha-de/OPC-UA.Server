#include "HaSubscriptionException.h"

using namespace CommonNamespace;

HaSubscriptionException::HaSubscriptionException(
        const std::string& message, const std::string& file, int line) :
Exception(message, file, line) {
}

Exception* HaSubscriptionException::copy() const {
    return new HaSubscriptionException(*this);
}
