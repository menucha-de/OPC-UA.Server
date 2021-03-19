#ifndef IODATAPROVIDER_SUBSCRIBERCALLBACKEXCEPTION_H_
#define IODATAPROVIDER_SUBSCRIBERCALLBACKEXCEPTION_H_

#include <common/Exception.h>
#include <string>

namespace IODataProviderNamespace {

class SubscriberCallbackException: public CommonNamespace::Exception {
public:
	// Copies of the given message and file path are saved internally.
	SubscriberCallbackException(const std::string& message,
			const std::string& file, int line);

	// interface Exception
	virtual Exception* copy() const;
};

} // namespace IODataProviderNamespace
#endif /* IODATAPROVIDER_SUBSCRIBERCALLBACKEXCEPTION_H_ */
