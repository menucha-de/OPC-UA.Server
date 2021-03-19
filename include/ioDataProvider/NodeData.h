#ifndef IODATAPROVIDER_NODEDATA_H_
#define IODATAPROVIDER_NODEDATA_H_

#include "IODataProviderException.h"
#include "NodeId.h"
#include "Variant.h"
#include <string>

namespace IODataProviderNamespace {

    class NodeDataPrivate;

    class NodeData {
    public:
        // If the values are attached then the responsibility for destroying the value instances
        // is delegated to the NodeData instance.
        NodeData(const NodeId& nodeId, const Variant* data, bool attachValues =
                false);
        // Creates a deep copy of the instance.
        NodeData(const NodeData& nodeData);
        virtual ~NodeData();

        virtual const NodeId& getNodeId() const;

        virtual const Variant* getData() const;
        virtual void setData(const Variant* data);

        virtual IODataProviderException* getException() const;
        virtual void setException(IODataProviderException* e);

        virtual std::string toString() const;
    private:
        NodeData& operator=(const NodeData&);

        NodeDataPrivate* d;
    };

} // namespace IODataProviderNamespace
#endif /* IODATAPROVIDER_NODEDATA_H_ */
