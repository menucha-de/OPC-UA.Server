#ifndef SASMODELPROVIDER_BASE_IODATAMANAGER_H_
#define SASMODELPROVIDER_BASE_IODATAMANAGER_H_

#include <iomanageruanode.h> // UaVariableArray

namespace SASModelProviderNamespace {

class IODataManager {
public:
	IODataManager();
	virtual ~IODataManager();

	// interface NodeManagerUaNode
	virtual UaStatus afterStartUp() = 0;
	virtual UaStatus beforeShutDown() = 0;

	// interface IOManagerUaNode
	virtual UaStatus readValues(const UaVariableArray &arrUaVariables,
			UaDataValueArray &arrDataValues) = 0;
	virtual UaStatus writeValues(const UaVariableArray &arrUaVariables,
			const PDataValueArray &arrpDataValues,
			UaStatusCodeArray &arrStatusCodes) = 0;
	virtual OpcUa_Boolean beforeSetAttributeValue(Session* pSession, UaNode* pNode,
			OpcUa_Int32 attributeId, const UaDataValue& dataValue, OpcUa_Boolean& checkWriteMask) = 0;
	virtual void afterSetAttributeValue(Session* pSession, UaNode* pNode,
			OpcUa_Int32 attributeId, const UaDataValue& dataValue) = 0;
	virtual void variableCacheMonitoringChanged(UaVariableCache* pVariable,
			IOManager::TransactionType transactionType) = 0;
};

} // namespace SASModelProviderNamespace
#endif /* SASMODELPROVIDER_BASE_IODATAMANAGER_H_ */
