#ifndef BINARYSERVER_HASESSIONEXCEPTION_H
#define BINARYSERVER_HASESSIONEXCEPTION_H

#include <common/Exception.h>
#include <string>

class HaSessionException : public CommonNamespace::Exception {
public:
    // Copies of the given message and file path are saved internally.
    HaSessionException(const std::string& message, const std::string& file, int line);

    // interface Exception
    virtual CommonNamespace::Exception* copy() const;
};

#endif /* BINARYSERVER_HASESSIONEXCEPTION_H */
