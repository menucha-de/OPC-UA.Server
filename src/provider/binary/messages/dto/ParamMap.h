#ifndef PROVIDER_BINARY_MESSAGES_DTO_PARAMMAP_H
#define PROVIDER_BINARY_MESSAGES_DTO_PARAMMAP_H

#include <map>
#include "ParamId.h"
#include "Variant.h"

class ParamMapPrivate;

class ParamMap {
public:
    // If the values are attached then the responsibility for destroying the value instances
    // is delegated to the instance.
    ParamMap(const std::map<const ParamId*, const Variant*>& elements,
            bool attachValues = false);
    // Creates a deep copy of the instance.
    ParamMap(const ParamMap& orig);    
    virtual ~ParamMap();

    virtual const std::map<const ParamId*, const Variant*>& getElements() const;
    virtual void setElements(
            const std::map<const ParamId*, const Variant*>& elements);
private:
    ParamMap& operator=(const ParamMap& orig);
    
    ParamMapPrivate* d;
};

#endif /* PROVIDER_BINARY_MESSAGES_DTO_PARAMMAP_H */
