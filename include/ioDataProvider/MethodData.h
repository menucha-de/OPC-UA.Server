#ifndef IODATAPROVIDER_METHODDATA_H_
#define IODATAPROVIDER_METHODDATA_H_

#include "IODataProviderException.h"
#include "NodeId.h"
#include "Scalar.h"
#include <vector>

namespace IODataProviderNamespace {

    class MethodDataPrivate;

    class MethodData {
    public:
        // The method arguments must be in the same order as described in the OPC UA Server Address Space.
        // If the values are attached then the responsibility for destroying the value instances
        // is delegated to the MethodData instance.
        MethodData(const NodeId& objectNodeId, const NodeId& methodNodeId,
                const std::vector<const Variant*>& methodArgs, bool attachValues =
                false);
        // Creates a deep copy of the instance.
        MethodData(const MethodData& methodData);
        virtual ~MethodData();

        virtual const NodeId& getObjectNodeId() const;
        virtual const NodeId& getMethodNodeId() const;
        virtual const std::vector<const Variant*>& getMethodArguments() const;
        // Replaces existing method arguments.
        // The method arguments must be in the same order as described in the OPC UA Server Address Space.
        virtual void setMethodArguments(const std::vector<const Variant*>& args);

        virtual IODataProviderException* getException() const;
        virtual void setException(IODataProviderException* e);
    private:
        MethodData& operator=(const MethodData&);

        MethodDataPrivate* d;
    };

} // namespace IODataProviderNamespace
#endif /* IODATAPROVIDER_METHODDATA_H_ */
