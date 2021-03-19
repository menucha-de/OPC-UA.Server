#ifndef IODATAPROVIDER_IODATAPROVIDEREXCEPTION_H_
#define IODATAPROVIDER_IODATAPROVIDEREXCEPTION_H_

#include <common/Exception.h>
#include <string>

namespace IODataProviderNamespace {

class IODataProviderException: public CommonNamespace::Exception {
public:
	// Copies of the given message and file path are saved internally.
	IODataProviderException(const std::string& message, const std::string& file,
			int line);

	// interface Exception
	virtual Exception* copy() const;
};

} // namespace IODataProviderNamespace
#endif /* IODATAPROVIDER_IODATAPROVIDEREXCEPTION_H_ */
