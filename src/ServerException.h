#ifndef SERVEREXCEPTION_H
#define SERVEREXCEPTION_H

#include <common/Exception.h>
#include <string>

class ServerException : public CommonNamespace::Exception {
public:
    // Copies of the given message and file path are saved internally.
    ServerException(const std::string& message, const std::string& file, int line);

    // interface Exception
    virtual CommonNamespace::Exception* copy() const;
};

#endif /* SERVEREXCEPTION_H */
