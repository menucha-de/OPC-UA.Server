#ifndef PROVIDER_BINARY_MESSAGES_DTO_PARAMID_H
#define PROVIDER_BINARY_MESSAGES_DTO_PARAMID_H

#include <string>
#include <vector>


class ParamIdPrivate;

class ParamId {
public:

    enum Type {
        NUMERIC = 0, //
        STRING = 1
    };

    ParamId(int namespaceIndex, unsigned long id);
    // If the values are attached then the responsibility for destroying the value instances
    // is delegated to the NodeId instance.
    ParamId(int namespaceIndex, const std::string& id, bool attachValues = false);
    // Creates a deep copy of the instance.
    ParamId(const ParamId& paramId);
    //From Full String  e.g. NS0|Numeric|1007
    ParamId(std::string fullString);
    virtual ~ParamId();

    virtual Type getParamIdType() const;
    const char* getParamIdTypeString() const;

    virtual int getNamespaceIndex() const;

    // 32 bit
    virtual unsigned long getNumeric() const;
    virtual const std::string& getString() const;

    virtual std::string toString() const;

private:    
    ParamId& operator=(const ParamId& orig);
    
    ParamIdPrivate* d;

    std::vector<std::string> split(const std::string& s, char d);


};

#endif /* PROVIDER_BINARY_MESSAGES_DTO_PARAMID_H */
