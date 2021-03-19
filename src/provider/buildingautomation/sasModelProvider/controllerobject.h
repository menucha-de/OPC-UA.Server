#ifndef PROVIDER_BUILDINGAUTOMATION_SASMODELPROVIDER_CONTROLLEROBJECT_H
#define PROVIDER_BUILDINGAUTOMATION_SASMODELPROVIDER_CONTROLLEROBJECT_H

#include <sasModelProvider/base/HaNodeManager.h>
#include <sasModelProvider/base/HaNodeManagerIODataProviderBridge.h>
#include <sasModelProvider/base/NodeBrowser.h>
#include "uaobjecttypes.h" // UaObjectBase
#include "uastring.h" // UaString
#include "methodmanager.h" // MethodManager
#include "uamutex.h" // UaMutexRefCounted
#include "uabasenodes.h" // UaVariable

class ControllerObject : public UaObjectBase {
    UA_DISABLE_COPY(ControllerObject);
public:
    ControllerObject(const UaString& name, const UaNodeId& newNodeId,
            const UaString& defaultLocaleId,
            SASModelProviderNamespace::HaNodeManager& haNodeManager,
            OpcUa_UInt32 deviceAddress);
    virtual ~ControllerObject(void);

    // interface UaObject
    virtual OpcUa_Byte eventNotifier() const;
    virtual MethodManager* getMethodManager(UaMethod* pMethod) const;
protected:
    UaMutexRefCounted* m_pSharedMutex;

    // If an object is found for the nodeId, the reference count of the object is 
    // incremented. The caller must release the reference with method "releaseReference"
    // when the object is no longer needed.
    virtual UaVariable* getVariable(OpcUa_UInt32 nodeId);
    // If an object is found for the nodeId, the reference count of the object is 
    // incremented. The caller must release the reference with method "releaseReference"
    // when the object is no longer needed.
    virtual UaMethod* getMethod(OpcUa_UInt32 nodeId);
private:
    SASModelProviderNamespace::HaNodeManager* haNodeManager;
    SASModelProviderNamespace::NodeBrowser* nodeBrowser;
};

#endif // PROVIDER_BUILDINGAUTOMATION_SASMODELPROVIDER_CONTROLLEROBJECT_H
