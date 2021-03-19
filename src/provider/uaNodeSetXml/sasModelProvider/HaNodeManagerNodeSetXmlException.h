#ifndef PROVIDER_UANODESETXML_SASMODELPROVIDER_HANODEMANAGERNODESETXMLEXCEPTION_H
#define PROVIDER_UANODESETXML_SASMODELPROVIDER_HANODEMANAGERNODESETXMLEXCEPTION_H

#include <common/Exception.h>
#include <string>

class HaNodeManagerNodeSetXmlException: public CommonNamespace::Exception {
public:
	// Copies of the given message and file path are saved internally.
	HaNodeManagerNodeSetXmlException(const std::string& message, const std::string& file,
			int line);

	// interface Exception
	virtual CommonNamespace::Exception* copy() const;
};

#endif /* PROVIDER_UANODESETXML_SASMODELPROVIDER_HANODEMANAGERNODESETXMLEXCEPTION_H */
