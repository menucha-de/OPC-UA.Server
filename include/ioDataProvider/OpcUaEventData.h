#ifndef IODATAPROVIDER_OPCUAEVENTDATA_H_
#define IODATAPROVIDER_OPCUAEVENTDATA_H_

#include "NodeData.h"
#include "Variant.h"
#include <string>
#include <vector>

namespace IODataProviderNamespace {

class OpcUaEventDataPrivate;

class OpcUaEventData: public Variant {
public:
	OpcUaEventData(const NodeId& sourceNodeId, const std::string& message,
			int severity, const std::vector<const NodeData*>& fieldData,
			bool attachValue = false);
	// Creates a deep copy of the OpcUaEventData instance.
	OpcUaEventData(const OpcUaEventData& eventData);
	virtual ~OpcUaEventData();

	virtual const NodeId& getSourceNodeId() const;
	// UTF-8, localeId "en"
	virtual const std::string& getMessage() const;
	virtual int getSeverity() const;
	virtual const std::vector<const NodeData*>& getFieldData() const;

	// interface Variant
	virtual Variant* copy() const;
	virtual Type getVariantType() const;
	virtual std::string toString() const;
private:
	OpcUaEventData& operator=(const OpcUaEventData&);

	OpcUaEventDataPrivate* d;
};

} // namespace IODataProviderNamespace
#endif /* IODATAPROVIDER_OPCUAEVENTDATA_H_ */
