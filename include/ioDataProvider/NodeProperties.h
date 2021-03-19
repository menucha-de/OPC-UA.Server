#ifndef IODATAPROVIDER_NODEPROPERTIES_H_
#define IODATAPROVIDER_NODEPROPERTIES_H_

#include "Variant.h"
#include <string>

namespace IODataProviderNamespace {

class NodePropertiesPrivate;

class NodeProperties: public Variant {
public:
	enum ValueHandling {
		NONE = 0, // The node does not require any value handling
		SYNC = 1, // The OPC UA server directly forwards all client requests to the IO data provider.
				  // For event handling the server polls for data changes.
		ASYNC = 2 // The OPC UA server subscribes for data changes. The IO data provider must sent
				  // events after data changes.
	};
	NodeProperties(ValueHandling valueHandling);
	// Creates a deep copy of the NodeProperties instance.
	NodeProperties(const NodeProperties& value);
	virtual ~NodeProperties();

	virtual ValueHandling getValueHandling() const;
	virtual void setValueHandling(ValueHandling valueHandling);

	// interface Variant
	virtual Variant* copy() const;
	virtual Type getVariantType() const;
	virtual std::string toString() const;
private:
	NodeProperties& operator=(const NodeProperties&);

	NodePropertiesPrivate* d;
};

} // namespace IODataProviderNamespace
#endif /* IODATAPROVIDER_NODEPROPERTIES_H_ */
