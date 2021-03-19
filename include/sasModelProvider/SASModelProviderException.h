#ifndef SASMODELPROVIDER_SASMODELPROVIDEREXCEPTION_H_
#define SASMODELPROVIDER_SASMODELPROVIDEREXCEPTION_H_

#include <common/Exception.h>
#include <string>

namespace SASModelProviderNamespace {

class SASModelProviderException: public CommonNamespace::Exception {
public:
	// Copies of the given message and file path are saved internally.
	SASModelProviderException(const std::string& message,
			const std::string& file, int line);

	// interface Exception
	virtual Exception* copy() const;
};

} // namespace SASModelProviderNamespace
#endif /* SASMODELPROVIDER_SASMODELPROVIDEREXCEPTION_H_ */
