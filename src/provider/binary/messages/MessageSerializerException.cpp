#include "MessageSerializerException.h"
#ifdef DEBUG
#include <CppUTest/MemoryLeakDetectorNewMacros.h>
#endif

using namespace CommonNamespace;

MessageSerializerException::MessageSerializerException(
		const std::string& message, const std::string& file, int line) :
		Exception(message, file, line) {
}

Exception* MessageSerializerException::copy() const {
	return new MessageSerializerException(*this);
}
