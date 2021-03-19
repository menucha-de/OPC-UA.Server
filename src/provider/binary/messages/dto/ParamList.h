#ifndef PROVIDER_BINARY_MESSAGES_DTO_PARAMLIST_H
#define PROVIDER_BINARY_MESSAGES_DTO_PARAMLIST_H

#include <vector>
#include "Variant.h"

class ParamListPrivate;

class ParamList {
public:
    // If the values are attached then the responsibility for destroying the value instances
    // is delegated to the instance.
    ParamList(const std::vector<const Variant*>& elements, bool attachValues =
            false);
    // Creates a deep copy of the instance.
    ParamList(const ParamList& paramList);    
    virtual ~ParamList();

    virtual const std::vector<const Variant*>& getElements() const;
    virtual void setElements(const std::vector<const Variant*>& elements);
private:    
    ParamList& operator=(const ParamList& orig);
    
    ParamListPrivate* d;
};

#endif /* PROVIDER_BINARY_MESSAGES_DTO_PARAMLIST_H */
