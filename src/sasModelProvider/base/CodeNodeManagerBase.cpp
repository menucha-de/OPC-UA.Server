#include <sasModelProvider/base/CodeNodeManagerBase.h>
#include <common/logging/Logger.h>
#include <common/logging/LoggerFactory.h>
#include <sasModelProvider/base/HaNodeManagerException.h>
#include <sstream> // std::ostringstream

using namespace CommonNamespace;

namespace SASModelProviderNamespace {

    class CodeNodeManagerBasePrivate {
        friend class CodeNodeManagerBase;
    private:
        Logger* log;

        EventTypeRegistry eventTypeRegistry;
        HaNodeManagerIODataProviderBridge* nmioBridge;
    };

    CodeNodeManagerBase::CodeNodeManagerBase(const UaString& sNamespaceUri,
            IODataProviderNamespace::IODataProvider& ioDataProvider,
            OpcUa_Boolean firesEvents) :
    NodeManagerBase(sNamespaceUri, firesEvents) {
        d = new CodeNodeManagerBasePrivate();
        d->log = LoggerFactory::getLogger("CodeNodeManagerBase");
        d->nmioBridge = new HaNodeManagerIODataProviderBridge(*this,
                ioDataProvider);
    }

    CodeNodeManagerBase::~CodeNodeManagerBase() {
        delete d->nmioBridge;
        delete d;
    }

    UaStatus CodeNodeManagerBase::afterStartUp() {
        return d->nmioBridge->afterStartUp();
    }

    UaStatus CodeNodeManagerBase::beforeShutDown() {
        return d->nmioBridge->beforeShutDown();
    }

    UaStatus CodeNodeManagerBase::readValues(const UaVariableArray& arrUaVariables,
            UaDataValueArray& arrDataValues) {
        return d->nmioBridge->readValues(arrUaVariables, arrDataValues);
    }

    UaStatus CodeNodeManagerBase::writeValues(const UaVariableArray& arrUaVariables,
            const PDataValueArray& arrpDataValues,
            UaStatusCodeArray& arrStatusCodes) {
        return d->nmioBridge->writeValues(arrUaVariables, arrpDataValues,
                arrStatusCodes);
    }

    void CodeNodeManagerBase::afterSetAttributeValue(Session* pSession,
            UaNode* pNode, OpcUa_Int32 attributeId, const UaDataValue& dataValue) {
        d->nmioBridge->afterSetAttributeValue(pSession, pNode, attributeId,
                dataValue);
    }

    void CodeNodeManagerBase::variableCacheMonitoringChanged(
            UaVariableCache* pVariable, TransactionType transactionType) {
        d->nmioBridge->variableCacheMonitoringChanged(pVariable, transactionType);
    }

    UaStatus CodeNodeManagerBase::beginCall(MethodManagerCallback* pCallback,
            const ServiceContext& serviceContext, OpcUa_UInt32 callbackHandle,
            MethodHandle* pMethodHandle, const UaVariantArray& inputArguments) {
        return d->nmioBridge->beginCall(pCallback, serviceContext, callbackHandle,
                pMethodHandle, inputArguments);
    }

    NodeManager& CodeNodeManagerBase::getNodeManagerRoot() {
        return *m_pServerManager->getNodeManagerRoot();
    }

    NodeManagerBase& CodeNodeManagerBase::getNodeManagerBase() {
        return *this;
    }

    const std::vector<HaNodeManager*>* CodeNodeManagerBase::getAssociatedNodeManagers() {
        return NULL;
    }

    EventTypeRegistry& CodeNodeManagerBase::getEventTypeRegistry() {
        return d->eventTypeRegistry;
    }

    HaNodeManagerIODataProviderBridge& CodeNodeManagerBase::getIODataProviderBridge() {
        return *d->nmioBridge;
    }

    UaString CodeNodeManagerBase::getNameSpaceUri() {
        return NodeManagerBase::getNameSpaceUri();
    }

    const UaString& CodeNodeManagerBase::getDefaultLocaleId() const {
        return m_defaultLocaleId;
    }

    void CodeNodeManagerBase::setVariable(UaVariable& variable,
            UaVariant& newValue) /* throws HaNodeManagerException */ {
        const OpcUa_Variant* cacheValue = variable.value(
                NULL /* session */).value();
        if (d->log->isInfoEnabled()) {
            d->log->info("SET %-20s %s -> %s", variable.nodeId().toXmlString().toUtf8(),
                    UaVariant(*cacheValue).toString().toUtf8(), newValue.toString().toUtf8());
        }
        // set new value
        UaDataValue dataValue;
        dataValue.setValue(newValue,
                OpcUa_False /* detachValue */,
                OpcUa_True /* updateTimeStamps */);
        UaStatus status = variable.setValue(NULL /* session */, dataValue,
                OpcUa_False /* checkAccessLevel */);
        if (!status.isGood()) {
            std::ostringstream msg;
            msg << "Cannot set value to variable: " << status.toString().toUtf8()
                    << " " << variable.nodeId().toXmlString().toUtf8() << " "
                    << UaVariant(*cacheValue).toString().toUtf8() << " -> "
                    << newValue.toString();
            throw ExceptionDef(HaNodeManagerException, msg.str());
        }
    }
} // namespace SASModelProviderNamespace
