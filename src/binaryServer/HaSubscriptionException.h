#ifndef BINARYSERVER_HASUBSCRIPTIONEXCEPTION_H_
#define BINARYSERVER_HASUBSCRIPTIONEXCEPTION_H_

#include <common/Exception.h>
#include <string>

class HaSubscriptionException : public CommonNamespace::Exception {
public:
    // Copies of the given message and file path are saved internally.
    HaSubscriptionException(const std::string& message, const std::string& file, int line);

    // interface Exception
    virtual CommonNamespace::Exception* copy() const;
};

#endif /* BINARYSERVER_HASUBSCRIPTIONEXCEPTION_H_ */
