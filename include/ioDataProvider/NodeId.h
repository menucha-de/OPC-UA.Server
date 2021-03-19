#ifndef IODATAPROVIDER_NODEID_H_
#define IODATAPROVIDER_NODEID_H_

#include "Variant.h"
#include <string>

namespace IODataProviderNamespace {

class NodeIdPrivate;

class NodeId : public Variant {
public:
	enum Type {
		NUMERIC, STRING
	};

	NodeId(int namespaceIndex, long id);
	// If the values are attached then the responsibility for destroying the value instances
	// is delegated to the NodeId instance.
	// id: UTF-8 encoded
	NodeId(int namespaceIndex, const std::string& id,
			bool attachValues = false);
	// Creates a deep copy of the instance.
	NodeId(const NodeId& nodeId);
	virtual ~NodeId();

	// Returns true if the namespace and the identifier are equal.
	virtual bool equals(const NodeId& nodeId) const;

	virtual Type getNodeType() const;

	virtual int getNamespaceIndex() const;
	virtual long getNumeric() const;
	virtual const std::string& getString() const;

        // interface Variant
        virtual Variant* copy() const;
        virtual Variant::Type getVariantType() const;
        virtual std::string toString() const;
private:
	NodeId& operator=(const NodeId&);

	NodeIdPrivate* d;
};

} // namespace IODataProviderNamespace
#endif /* IODATAPROVIDER_NODEID_H_ */
