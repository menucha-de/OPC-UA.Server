#include "HaSession.h"
#include "Event.h"
#include "EventField.h"
#include "HaSessionException.h"
#include "HaSubscription.h"
#include "HaSubscriptionException.h"
#include "../provider/binary/common/TimeoutException.h"
#include <common/Exception.h>
#include <common/Mutex.h>
#include <common/MutexLock.h>
#include <common/ScopeGuard.h>
#include <common/VectorScopeGuard.h>
#include <common/logging/Logger.h>
#include <common/logging/LoggerFactory.h>
#include <statuscode.h> // UaStatus
#include <uaargument.h> // UaArguments
#include <uaarraytemplates.h> // UaStringArray
#include <uabytestring.h> // UaByteString
#include <uaclientsdk.h> // UaSessionCallback
#include <uadatetime.h> // UaDateTime
#include <uadiagnosticinfos.h> // UaDiagnosticInfos
#include <uaqualifiedname.h> // UaQualifiedName
#include <uanodeid.h> // UaNodeId
#include <uaplatformdefs.h> // UA_GetHostname
#include <uasession.h> // UaSession
#include <uastring.h> // UaString
#include <uastructuredefinition.h> // UaStructureDefinition
#include <uavariant.h> // UaVariant
#include <ctime> // std::time_t
#include <sstream> // std::ostringstream
#include <time.h> // nanosleep

using namespace CommonNamespace;

class HaSessionPrivate: public UaClientSdk::UaSessionCallback {
	friend class HaSession;
private:

	class HaSessionSubscriptionCallback: public HaSubscription::HaSubscriptionCallback {
	public:

		HaSessionSubscriptionCallback(HaSession::HaSessionCallback& callback) {
			this->callback = &callback;
		}

		virtual void dataChanged(std::vector<NodeAttributes*>& nodeAttributes)
		/*throws Exception */{
			try {
				// fire event
				callback->dataChanged(nodeAttributes); // any exception
			} catch (Exception& e) {
				HaSessionException ex = ExceptionDef(HaSessionException,
						std::string("Cannot send data change notification"));
				ex.setCause(&e);
				throw ex;
			}
		}

		virtual void newEvents(
				std::vector<const BinaryServerNamespace::Event*>& events)
				/* throws Exception */{
			try {
				// fire event
				callback->newEvents(events); // any exception
			} catch (Exception& e) {
				HaSessionException ex = ExceptionDef(HaSessionException,
						std::string("Cannot send events"));
				ex.setCause(&e);
				throw ex;
			}
		}
	private:
		HaSession::HaSessionCallback* callback;
	};

	// Sdk 1.5.2: uaclient/uaclientcpp/uasession.cpp:
	//   The SDK is managing the connection to the server by
	//   - Monitoring the status of the session with read calls to the server status variable. The frequency of the read calls can be
	//     controlled with the setting SessionConnectInfo::nWatchdogTime
	//   - Reconnect on TCP/IP or SecureChannel level if the connection was lost
	//   - Recreation of the session if the session timed out or the server was restarted
	//   - The callback function UaSessionCallback::connectionStatusChanged provides information about the current status of the session
	static const OpcUa_Double SESSION_TIMEOUT = 3600000; // in ms

	Logger* log;
	HaSession* parent;
	HaSession::Configuration* conf;
	HaSession::HaSessionCallback* callback;

	UaString applicationName;
	UaString serverUrl;
	UaString applicationUri;
	UaString productUri;

	UaClientSdk::UaSession* session;
	HaSubscription* subscription;
	HaSessionSubscriptionCallback* subscriptionCallback;
	Mutex* mutex;

	void read(const std::vector<const UaNodeId*>& nodeIds,
			std::vector<OpcUa_UInt32>& attributeIds,
			std::vector<NodeAttributes*>& returnNodeAttributes) /* throws HaSessionException */;
	void getEventFields(const UaNodeId& eventTypeId,
			std::vector<BinaryServerNamespace::EventField*>& returnEventFields)
			/*throws HaSessionException*/;

	// interface UaSessionCallback
	virtual void connectionStatusChanged(OpcUa_UInt32 clientConnectionId,
			UaClientSdk::UaClient::ServerStatus serverStatus);
};

HaSession::HaSessionCallback::HaSessionCallback() {
}

HaSession::HaSessionCallback::~HaSessionCallback() {
}

HaSession::HaSession(HaSession::Configuration& conf,
		HaSession::HaSessionCallback& callback)
		/* throws MutexException */{
	d = new HaSessionPrivate();
	d->log = LoggerFactory::getLogger("HaSession");
	d->parent = this;
	d->conf = &conf;
	d->callback = &callback;

	d->applicationName = UaString("binaryServer");
	UaString companyName("harting");
	UaString productName("binaryServer");

	if (d->conf->port != 0) {
		d->serverUrl = UaString("opc.tcp://%1:%2").arg(
				UaString(d->conf->host.c_str())).arg(d->conf->port);
	} else {
		d->serverUrl = UaString("opc.tcp://%1").arg(
				UaString(d->conf->host.c_str()));
	}

	UaString nodeName("unknown_host");
	char szHostName[256];
	if (0 == UA_GetHostname(szHostName, 256)) {
		nodeName = szHostName;
	}
	d->applicationUri =
			UaString("urn:%1:%2:%3").arg(nodeName).arg(companyName).arg(
					productName);
	d->productUri = UaString("urn:%1:%2").arg(companyName).arg(productName);
	d->subscriptionCallback =
			new HaSessionPrivate::HaSessionSubscriptionCallback(callback);
	d->mutex = new Mutex(); // MutexException
}

HaSession::~HaSession() /* throws HaSessionException */{
	close();
	delete d->mutex;
	delete d->subscriptionCallback;
	delete d;
}

void HaSession::open() /* throws HaSessionException */{
	MutexLock lock(*d->mutex);
	if (d->session != NULL) {
		return;
	}
	// create a new UaSession
	d->session = new UaClientSdk::UaSession();
	d->subscription = new HaSubscription(*d->session,
			d->conf->sendReceiveTimeout, d->conf->publishingInterval,
			*d->subscriptionCallback);

	UaClientSdk::SessionConnectInfo sessionConnectInfo;
	sessionConnectInfo.clientConnectionId = 0;
	sessionConnectInfo.sLocaleId = "en";
	sessionConnectInfo.sApplicationName = d->applicationName;
	sessionConnectInfo.sApplicationUri = d->applicationUri;
	sessionConnectInfo.sProductUri = d->productUri;
	sessionConnectInfo.sSessionName = sessionConnectInfo.sApplicationUri;
	sessionConnectInfo.nSessionTimeout = d->SESSION_TIMEOUT;
	sessionConnectInfo.nWatchdogTime = d->conf->watchdogInterval * 1000;
	sessionConnectInfo.nWatchdogTimeout = d->conf->sendReceiveTimeout * 1000;
	sessionConnectInfo.bAutomaticReconnect = true;
	sessionConnectInfo.bRetryInitialConnect = false;

	UaClientSdk::SessionSecurityInfo sessionSecurityInfo;
	if (d->conf->username != NULL) {
		sessionSecurityInfo.setUserPasswordUserIdentity(
				UaString(d->conf->username->c_str()),
				UaString(d->conf->password->c_str()));
	}

	timespec reconnectDelay;
	reconnectDelay.tv_sec = 0;
	reconnectDelay.tv_nsec = 0;
	std::time_t time = std::time(NULL);
	std::time_t endTime = time + d->conf->connectTimeout;
	std::time_t remainingTimeout = endTime - time;
	UaStatus result(OpcUa_Bad);
	try {
		while (!result.isGood()) {
			if (remainingTimeout <= 0) {
				std::ostringstream msg;
				msg << "Time out after " << d->conf->connectTimeout << " sec.";
				throw ExceptionDef(TimeoutException, msg.str());
			}
			sessionConnectInfo.nConnectTimeout = remainingTimeout * 1000;
			result = d->session->connect(d->serverUrl, sessionConnectInfo,
					sessionSecurityInfo, d /* UaSessionCallback */);
			if (result.isGood()) {
				// connection has been successfully created => reset reconnect delay
				reconnectDelay.tv_sec = 0;
			} else {
				if (d->log->isWarnEnabled()) {
					d->log->warn("Cannot open a connection to %s: %s",
							d->serverUrl.toUtf8(), result.toString().toUtf8());
				}
				// wait some time
				if (reconnectDelay.tv_sec < d->conf->maxReconnectDelay) {
					reconnectDelay.tv_sec++;
				}
				remainingTimeout = endTime - std::time(NULL);
				if (reconnectDelay.tv_sec > remainingTimeout) {
					reconnectDelay.tv_sec = remainingTimeout;
				}
				d->log->debug("Waiting %ld seconds...", reconnectDelay.tv_sec);
				nanosleep(&reconnectDelay, NULL /* remaining */);
				remainingTimeout -= reconnectDelay.tv_sec;
			}
		}
		d->log->debug(
				"Opened connection to %s (sessionId=%s,revisedSessionTimeout=%d (s),revisedWatchdogInterval=%d (s), sendReceiveTimeout=%d (s), publishInterval=%d (ms), connectTimeout=%d (s))",
				d->serverUrl.toUtf8(),
				d->session->sessionId().toXmlString().toUtf8(),
				(int)(d->session->revisedSessionTimeout() / 1000.0),
				d->session->watchdogTime() / 1000,
				d->conf->sendReceiveTimeout,
				d->conf->publishingInterval,
				d->conf->connectTimeout);
	} catch (Exception& e) {
		delete d->session;
		d->session = NULL;
		// convert exception to IODataProviderException
		std::ostringstream msg;
		msg << "Cannot open a connection to " << d->serverUrl.toUtf8()
				<< " within " << d->conf->connectTimeout << " seconds";
		HaSessionException ex = ExceptionDef(HaSessionException, msg.str());
		ex.setCause(&e);
		throw ex;
	}
}

std::string HaSession::getSessionId() {
	return d->session->sessionId().toXmlString().toUtf8();
}

void HaSession::close() /* throws HaSessionException, HaSubscriptionException */{
	MutexLock lock(*d->mutex);
	if (d->session == NULL) {
		return;
	}
	// delete subscription
	delete d->subscription; // HaSubscriptionException
	d->subscription = NULL;
	// disconnect
	UaClientSdk::ServiceSettings serviceSettings;
	serviceSettings.callTimeout = d->conf->sendReceiveTimeout * 1000;
	UaStatus result = d->session->disconnect(serviceSettings,
			OpcUa_True /* deleteSubscriptions*/);
	if (result.isGood()) {
		if (d->log->isDebugEnabled()) {
			d->log->debug("Closed connection %s to %s",
					d->session->sessionId().toXmlString().toUtf8(),
					d->serverUrl.toUtf8());
		}
		delete d->session;
		d->session = NULL;
	} else {
		std::ostringstream msg;
		msg << "Cannot close the connection to " << d->serverUrl.toUtf8()
				<< ": " << result.toString().toUtf8();
		throw ExceptionDef(HaSessionException, msg.str());
	}
}

UaStringArray HaSession::getNamespaceTable() const {
	MutexLock lock(*d->mutex);
	if (d->session == NULL) {
		throw ExceptionDef(HaSessionException,
				std::string(
						"Cannot get namespace table due to closed session"));
	}
	return d->session->getNamespaceTable();
}

UaStructureDefinition HaSession::getStructureDefinition(
		const UaNodeId& dataTypeId) {
	MutexLock lock(*d->mutex);
	if (d->session == NULL) {
		throw ExceptionDef(HaSessionException,
				std::string(
						"Cannot get structure definition due to closed session"));
	}
	return d->session->structureDefinition(dataTypeId);
}

std::vector<UaNodeId>* HaSession::getSuperTypes(const UaNodeId& typeId)
/*throws HaSessionException*/{
	MutexLock lock(*d->mutex);
	if (d->session == NULL) {
		throw ExceptionDef(HaSessionException,
				std::string("Cannot get super types due to closed session"));
	}
	std::vector<UaNodeId>* ret = new std::vector<UaNodeId>();

	UaClientSdk::ServiceSettings serviceSettings;
	serviceSettings.callTimeout = d->conf->sendReceiveTimeout * 1000;

	UaClientSdk::BrowseContext browseContext;
	browseContext.maxReferencesToReturn = 0;
	browseContext.browseDirection = OpcUa_BrowseDirection_Inverse;
	browseContext.referenceTypeId = OpcUaId_HasSubtype;
	browseContext.includeSubtype = OpcUa_True;
	browseContext.nodeClassMask = 0;

	UaNodeId nodeId = typeId;
	while (!nodeId.isNull() && nodeId.namespaceIndex() != 0) {
		UaByteString continuationPoint;
		UaReferenceDescriptions referenceDescriptions;
		UaStatus result = d->session->browse(serviceSettings, nodeId,
				browseContext, continuationPoint, referenceDescriptions);
		if (!result.isGood()) {
			std::ostringstream msg;
			msg << "Cannot get super types for "
					<< typeId.toXmlString().toUtf8() << ": "
					<< result.toString().toUtf8();
			throw ExceptionDef(HaSessionException, msg.str());
		}
		if (referenceDescriptions.length() > 0) {
			for (OpcUa_UInt32 i = 0; i < referenceDescriptions.length(); i++) {
				nodeId = UaNodeId(referenceDescriptions[i].NodeId.NodeId);
				ret->push_back(nodeId);
			}
		} else {
			nodeId = UaNodeId();
		}
	}
	return ret;
}

void HaSession::getMethodArguments(const UaNodeId& methodId,
		UaArguments& returnInputArguments,
		UaArguments& returnOutputArguments) /* throws HaSessionException */{
	MutexLock lock(*d->mutex);
	if (d->session == NULL) {
		throw ExceptionDef(HaSessionException,
				std::string(
						"Cannot get method arguments due to closed session"));
	}
	UaClientSdk::ServiceSettings serviceSettings;
	serviceSettings.callTimeout = d->conf->sendReceiveTimeout * 1000;
	if (d->log->isDebugEnabled()) {
		d->log->debug("Getting method arguments for %s",
				methodId.toXmlString().toUtf8());
	}
	UaStatus result = d->session->getMethodArguments(serviceSettings, methodId,
			returnInputArguments, returnOutputArguments);
	if (d->log->isDebugEnabled()) {
		for (int i = 0; i < returnInputArguments.length(); i++) {
			UaArgument arg(returnInputArguments[i]);
			d->log->debug("Get method input argument %-20s dataTypeId=%s",
					arg.getName().toUtf8(),
					arg.getDataType().toXmlString().toUtf8());
		}
		for (int i = 0; i < returnOutputArguments.length(); i++) {
			UaArgument arg(returnOutputArguments[i]);
			d->log->debug("Get method output argument %-20s dataTypeId=%s",
					arg.getName().toUtf8(),
					arg.getDataType().toXmlString().toUtf8());
		}
	}
	if (!result.isGood()) {
		std::ostringstream msg;
		msg << "Cannot get method arguments for "
				<< methodId.toXmlString().toUtf8() << ": "
				<< result.toString().toUtf8();
		throw ExceptionDef(HaSessionException, msg.str());
	}
}



void HaSession::read(const std::vector<const UaNodeId*>& nodeIds,
		std::vector<NodeAttributes*>& returnNodeAttributes) /* throws HaSessionException */{
	if (nodeIds.size() == 0) {
		return;
	}
	MutexLock lock(*d->mutex);
	if (d->session == NULL) {
		throw ExceptionDef(HaSessionException,
				std::string("Cannot read due to closed session"));
	}
	// get node classes
	std::vector<OpcUa_UInt32> attributeIds;
	attributeIds.push_back(OpcUa_Attributes_NodeClass);

	//test code for registerNodes (the following read call must be deactivated)
	//    UaClientSdk::ServiceSettings registerServiceSettings;
	//    UaNodeIdArray nodesToRegister;
	//    nodesToRegister.create(nodeIds.size());
	//    for (OpcUa_Int16 i = 0; i < nodeIds.size(); i++) {
	//        nodeIds.at(i)->copyTo(&nodesToRegister[i]);
	//        d->log->debug("Register node %s", nodeIds.at(i)->toXmlString().toUtf8());
	//    }
	//    UaNodeIdArray registeredNodes;
	//    UaStatus registerResult = d->session->registerNodes(registerServiceSettings, nodesToRegister, registeredNodes);
	//    if (!registerResult.isGood()) {
	//        throw ExceptionDef(HaSessionException, std::string("Cannot register nodes"));
	//    }
	//    std::vector<const UaNodeId*>* registeredNodeIds = new std::vector<const UaNodeId*>();
	//    VectorScopeGuard<const UaNodeId> registeredNodeIdsSG(registeredNodeIds);
	//    for (OpcUa_Int16 i = 0; i < registeredNodes.length(); i++) {
	//        registeredNodeIds->push_back(new UaNodeId(registeredNodes[i]));
	//        d->log->debug("Registered node %s", registeredNodeIds->at(i)->toXmlString().toUtf8());
	//    }
	//    d->read(*registeredNodeIds, attributeIds, returnNodeAttributes); // HaSessionException

	d->read(nodeIds, attributeIds, returnNodeAttributes); // HaSessionException

	// nodeIdXml -> node attributes
	std::map<std::string, NodeAttributes*> retNodeAttrMap;

	// get readable nodes (without event types)
	std::vector<const UaNodeId*> readableNodeIds;
	// for each node
	for (int i = 0; i < returnNodeAttributes.size(); i++) {
		NodeAttributes& nodeAttr = *returnNodeAttributes[i];
		if (nodeAttr.getException() == NULL) {
			// if event type
			if (*nodeAttr.getNodeClass() == OpcUa_NodeClass_ObjectType) {
				// return any value
				UaVariant* var = new UaVariant();
				var->setByte(1);
				nodeAttr.setValue(var);
				nodeAttr.setDataType(new UaNodeId(OpcUaType_Byte));
			} else {
				readableNodeIds.push_back(&nodeAttr.getNodeId());
			}
			retNodeAttrMap[nodeAttr.getNodeId().toXmlString().toUtf8()] =
					&nodeAttr;
		}
	}

	try {
		// read values and their data type
		if (readableNodeIds.size() > 0) {
			attributeIds.clear();
			attributeIds.push_back(OpcUa_Attributes_Value);
			attributeIds.push_back(OpcUa_Attributes_DataType);
			std::vector<NodeAttributes*>* values = new std::vector<
					NodeAttributes*>();
			d->read(readableNodeIds, attributeIds, *values); // HaSessionException
			VectorScopeGuard<NodeAttributes> valuesSG(values);
			for (int i = 0; i < values->size(); i++) {
				NodeAttributes& nodeAttr = *(*values)[i];
				NodeAttributes& retNodeAttr =
						*retNodeAttrMap[nodeAttr.getNodeId().toXmlString().toUtf8()];
				if (nodeAttr.getValue() != NULL) {
					retNodeAttr.setValue(new UaVariant(*nodeAttr.getValue()));
				}
				if (nodeAttr.getDataType() != NULL) {
					retNodeAttr.setDataType(
							new UaNodeId(*nodeAttr.getDataType()));
				}
				if (nodeAttr.getException() != NULL) {
					retNodeAttr.setException(nodeAttr.getException()->copy());
				}
			}
		}
	} catch (Exception& e) {
		// remove added node attributes
		for (int i = 0; i < returnNodeAttributes.size(); i++) {
			delete returnNodeAttributes[i];
		}
		returnNodeAttributes.clear();
		throw;
	}
}

void HaSession::write(const std::map<const UaNodeId*, const UaVariant*>& nodes)
/* throws HaSessionException */{
	if (nodes.size() == 0) {
		return;
	}
	MutexLock lock(*d->mutex);
	if (d->session == NULL) {
		throw ExceptionDef(HaSessionException,
				std::string("Cannot write data to closed session"));
	}

	//test code for registerNodes (the following block creating UaWriteValues must be deactivated)
	//    UaClientSdk::ServiceSettings registerServiceSettings;
	//    UaNodeIdArray nodesToRegister;
	//    nodesToRegister.create(nodes.size());
	//    int i = 0;
	//    for (std::map<const UaNodeId*, const UaVariant*>::const_iterator it = nodes.begin();
	//            it != nodes.end(); it++) {
	//        const UaNodeId& nodeId = *it->first;
	//        nodeId.copyTo(&nodesToRegister[i]);
	//        d->log->debug("Register node %s", nodeId.toXmlString().toUtf8());
	//        i++;
	//    }
	//    UaNodeIdArray registeredNodes;
	//    UaStatus registerResult = d->session->registerNodes(registerServiceSettings, nodesToRegister, registeredNodes);
	//    if (!registerResult.isGood()) {
	//        throw ExceptionDef(HaSessionException, std::string("Cannot register nodes"));
	//    }
	//    UaWriteValues attrToWrite;
	//    attrToWrite.create(nodes.size());
	//    int index = 0;
	//    for (std::map<const UaNodeId*, const UaVariant*>::const_iterator it = nodes.begin();
	//            it != nodes.end(); it++) {
	//        const UaNodeId& nodeId = registeredNodes[index];
	//        const UaVariant& value = *it->second;
	//        attrToWrite[index].AttributeId = OpcUa_Attributes_Value;
	//        nodeId.copyTo(&attrToWrite[index].NodeId);
	//        value.copyTo(&attrToWrite[index].Value.Value);
	//        index++;
	//    }

	UaWriteValues attrToWrite;
	attrToWrite.create(nodes.size());
	int index = 0;
	for (std::map<const UaNodeId*, const UaVariant*>::const_iterator it =
			nodes.begin(); it != nodes.end(); it++) {
		const UaNodeId& nodeId = *it->first;
		const UaVariant& value = *it->second;
		attrToWrite[index].AttributeId = OpcUa_Attributes_Value;
		nodeId.copyTo(&attrToWrite[index].NodeId);
		value.copyTo(&attrToWrite[index].Value.Value);
		index++;
	}

	// write values
	UaClientSdk::ServiceSettings serviceSettings;
	serviceSettings.callTimeout = d->conf->sendReceiveTimeout * 1000;
	UaStatusCodeArray results;
	UaDiagnosticInfos diagnosticInfos;
	d->log->debug("Writing %d node attributes", attrToWrite.length());
	UaStatus result = d->session->write(serviceSettings, attrToWrite, results,
			diagnosticInfos);
	if (!result.isGood()) {
		std::ostringstream msg;
		msg << "Cannot read values: " << result.toString().toUtf8();
		throw ExceptionDef(HaSessionException, msg.str());
	}
	d->log->debug("Wrote %d node attributes\n", results.length());
	// for each result
	for (OpcUa_UInt32 i = 0; i < results.length(); i++) {
		UaNodeId nodeId(attrToWrite[i].NodeId);
		UaStatus status(results[i]);
		if (!status.isGood()) {
			std::ostringstream msg;
			msg << "Cannot write value for " << nodeId.toXmlString().toUtf8()
					<< ": " << status.toString().toUtf8();
			throw ExceptionDef(HaSessionException, msg.str());
		}
		if (d->log->isDebugEnabled()) {
			d->log->debug("Wrote %s value=%s", nodeId.toXmlString().toUtf8(),
					UaVariant(attrToWrite[i].Value.Value).toFullString().toUtf8());
		}
	}
}

UaStatus HaSession::browse(ServiceSettings &serviceSettings,
		const UaNodeId &startingNode, BrowseContext &browseContext,
		UaByteString &continuationPoint,
		UaReferenceDescriptions &referenceDescriptions) {
	UaStatus state = d->session->browse(serviceSettings, startingNode,
			browseContext, continuationPoint, referenceDescriptions);
	return state;
}

UaStatus HaSession::browseNext(ServiceSettings &settings, OpcUa_Boolean release,
		UaByteString &continuationPoint,
		UaReferenceDescriptions &referenceDescriptions) {
	return d->session->browseNext(settings, release, continuationPoint,
			referenceDescriptions);
}

std::vector<UaVariant*>* HaSession::call(const UaNodeId& methodId,
		const UaNodeId& objectId,
		const std::vector<UaVariant*>& inputArguments) /*throws HaSessionException*/{
	MutexLock lock(*d->mutex);
	if (d->session == NULL) {
		throw ExceptionDef(HaSessionException,
				std::string("Cannot call method due to closed session"));
	}
	UaClientSdk::ServiceSettings serviceSettings;
	serviceSettings.callTimeout = d->conf->sendReceiveTimeout * 1000;
	if (d->log->isDebugEnabled()) {
		d->log->debug(
				"Calling method %s on object %s with %d input arguments...",
				methodId.toXmlString().toUtf8(),
				objectId.toXmlString().toUtf8(), inputArguments.size());
	}
	UaClientSdk::CallIn callRequest;
	callRequest.methodId = methodId;
	callRequest.objectId = objectId;
	callRequest.inputArguments.create(inputArguments.size());
	for (int i = 0; i < inputArguments.size(); i++) {
		inputArguments[i]->copyTo(&callRequest.inputArguments[i]);
		if (d->log->isDebugEnabled()) {
			d->log->debug("Sending method input argument value=%s",
					inputArguments[i]->toFullString().toUtf8());
		}
	}
	UaClientSdk::CallOut callResult;
	UaStatus result = d->session->call(serviceSettings, callRequest,
			callResult);
	// if calling failed
	if (!result.isGood()) {
		std::ostringstream msg;
		msg << "Cannot call method " << methodId.toXmlString().toUtf8()
				<< " on object " << objectId.toXmlString().toUtf8() << ": "
				<< result.toString().toUtf8();
		HaSessionException ex = ExceptionDef(HaSessionException, msg.str());
		unsigned long errorCode = result.code();
		ex.setErrorCode(&errorCode);
		throw ex;
	}
	if (!callResult.callResult.isGood()) {
		std::ostringstream msg;
		msg << "Cannot call method " << methodId.toXmlString().toUtf8()
				<< " on object " << objectId.toXmlString().toUtf8() << ": "
				<< callResult.callResult.toString().toUtf8();
		HaSessionException ex = ExceptionDef(HaSessionException, msg.str());
		unsigned long errorCode = callResult.callResult.code();
		ex.setErrorCode(&errorCode);
		throw ex;
	}
	if (d->log->isDebugEnabled()) {
		d->log->debug("Called method returned with %d output arguments",
				callResult.outputArguments.length());
	}
	std::vector<UaVariant*>* ret = new std::vector<UaVariant*>();
	if (callResult.outputArguments.length() > 0) {
		for (OpcUa_UInt32 i = 0; i < callResult.outputArguments.length(); i++) {
			ret->push_back(new UaVariant(callResult.outputArguments[i]));
			if (d->log->isDebugEnabled()) {
				d->log->debug("Get method output argument value=%s",
						ret->back()->toFullString().toUtf8());
			}
		}
	}
	return ret;
}

void HaSession::subscribe(const std::vector<const UaNodeId*>& nodeIds,
		std::vector<NodeAttributes*>& returnNodeAttributes,
		std::map<UaNodeId, std::vector<BinaryServerNamespace::EventField*>*>& returnEventFields)
		/* throws HaSubscriptionException */{
	MutexLock lock(*d->mutex);
	if (d->session == NULL) {
		throw ExceptionDef(HaSessionException,
				std::string("Cannot subscribe due to closed session"));
	}

	//test code for registerNodes(the following read call must be deactivated)
	//    UaClientSdk::ServiceSettings registerServiceSettings;
	//    UaNodeIdArray nodesToRegister;
	//    nodesToRegister.create(nodeIds.size());
	//    for (OpcUa_Int16 i = 0; i < nodeIds.size(); i++) {
	//        nodeIds.at(i)->copyTo(&nodesToRegister[i]);
	//        d->log->debug("Register node %s", nodeIds.at(i)->toXmlString().toUtf8());
	//    }
	//    UaNodeIdArray registeredNodes;
	//    UaStatus registerResult = d->session->registerNodes(registerServiceSettings, nodesToRegister, registeredNodes);
	//    if (!registerResult.isGood()) {
	//        throw ExceptionDef(HaSessionException, std::string("Cannot register nodes"));
	//    }
	//    std::vector<const UaNodeId*>* registeredNodeIds = new std::vector<const UaNodeId*>();
	//    VectorScopeGuard<const UaNodeId> registeredNodeIdsSG(registeredNodeIds);
	//    for (OpcUa_Int16 i = 0; i < registeredNodes.length(); i++) {
	//        registeredNodeIds->push_back(new UaNodeId(registeredNodes[i]));
	//        d->log->debug("Registered node %s", registeredNodeIds->at(i)->toXmlString().toUtf8());
	//    }
	//    read(*registeredNodeIds, returnNodeAttributes); // HaSessionException

	// read node attributes
	read(nodeIds, returnNodeAttributes); // HaSessionException

	// read event fields
	HaSessionException* exception = NULL;
	std::vector<NodeAttributes*> validNodeAttributes;
	// for each node
	for (int i = 0; i < returnNodeAttributes.size(); i++) {
		NodeAttributes* attr = returnNodeAttributes[i];
		try {
			// if reading of attributes failed
			if (attr->getException() != NULL) {
				throw *attr->getException();
			}
			// if node is an event type
			if (*attr->getNodeClass() == OpcUa_NodeClass_ObjectType) {
				// get event fields
				std::vector<BinaryServerNamespace::EventField*>* evFields =
						new std::vector<BinaryServerNamespace::EventField*>();
				VectorScopeGuard<BinaryServerNamespace::EventField> evFieldsSG(
						evFields);
				if (d->log->isDebugEnabled()) {
					d->log->debug("Getting event fields for %s...",
							attr->getNodeId().toXmlString().toUtf8());
				}
				d->getEventFields(attr->getNodeId(), *evFields); // HaSessionException
				if (d->log->isDebugEnabled()) {
					for (int i = 0; i < evFields->size(); i++) {
						BinaryServerNamespace::EventField* evField =
								(*evFields)[i];
						d->log->debug(
								"Get event field %-20s nodeId=%s,dataType=%s",
								evField->getQualifiedName()->toString().toUtf8(),
								evField->getNodeId().toXmlString().toUtf8(),
								evField->getDataTypeId()->toXmlString().toUtf8());
					}
				}
				// for each event field
				for (int i = 0; i < evFields->size(); i++) {
					BinaryServerNamespace::EventField& evField = *(*evFields)[i];
					// if an exception has occurred
					if (evField.getException() != NULL) {
						throw *evField.getException();
					}
				}
				returnEventFields[attr->getNodeId()] = evFieldsSG.detach();
			}
			validNodeAttributes.push_back(attr);
		} catch (Exception& e) {
			if (exception == NULL) {
				std::ostringstream msg;
				msg << "Cannot add node "
						<< attr->getNodeId().toXmlString().toUtf8()
						<< " to subscription";
				exception = new ExceptionDef(HaSessionException, msg.str());
				exception->setCause(&e);
			}
		}
	}
	if (validNodeAttributes.size() > 0) {
		try {
			// add nodes to subscription
			d->subscription->add(validNodeAttributes, returnEventFields); // HaSubscriptionException
		} catch (Exception& e) {
			if (exception == NULL) {
				exception = new ExceptionDef(HaSessionException,
						std::string("Cannot add nodes to subscription"));
				exception->setCause(&e);
			}
		}
	}
	if (exception != NULL) {
		// delete node attributes
		for (int i = 0; i < returnNodeAttributes.size(); i++) {
			delete returnNodeAttributes[i];
		}
		returnNodeAttributes.clear();
		// delete eventField structure
		for (std::map<UaNodeId, std::vector<BinaryServerNamespace::EventField*>*>::const_iterator it =
				returnEventFields.begin(); it != returnEventFields.end();
				it++) {
			VectorScopeGuard<BinaryServerNamespace::EventField> eventFieldsSG(
					it->second);
		}
		returnEventFields.clear();
		ScopeGuard<HaSessionException> exceptionSG(exception);
		throw *exception;
	}
}

void HaSession::unsubscribe(
		const std::vector<const UaNodeId*>& nodeIds) /* throws HaSubscriptionException */{
	MutexLock lock(*d->mutex);
	if (d->subscription != NULL) {
		d->subscription->remove(nodeIds);
	}
}

void HaSession::unsubscribe() /* throws HaSubscriptionException */{
	MutexLock lock(*d->mutex);
	if (d->subscription != NULL) {
		d->subscription->removeAll();
	}
}

void HaSessionPrivate::read(const std::vector<const UaNodeId*>& nodeIds,
		std::vector<OpcUa_UInt32>& attributeIds,
		std::vector<NodeAttributes*>& returnNodeAttributes)
		/* throws HaSessionException */{
	if (nodeIds.size() == 0) {
		return;
	}

	UaReadValueIds attrToRead;
	attrToRead.create(nodeIds.size() * attributeIds.size());
	// nodeToRead index -> ret index
	std::map<OpcUa_UInt32, OpcUa_UInt32> indexMap;
	OpcUa_UInt32 attrToReadIndex = 0;
	// for each node
	for (OpcUa_UInt32 retIndex = 0; retIndex < nodeIds.size(); retIndex++) {
		const UaNodeId& nodeId = *nodeIds[retIndex];
		// for each attribute
		for (int i = 0; i < attributeIds.size(); i++) {
			attrToRead[attrToReadIndex].AttributeId = attributeIds[i];
			nodeId.copyTo(&attrToRead[attrToReadIndex].NodeId);
			indexMap[attrToReadIndex++] = retIndex;
		}
		// add empty object for attribute values to return array
		returnNodeAttributes.push_back(
				new NodeAttributes(*new UaNodeId(nodeId),
						true /*attachValues*/));
	}
	// if nothing has to be read
	if (attrToReadIndex == 0) {
		return;
	}

	// read data
	attrToRead.resize(attrToReadIndex);
	UaClientSdk::ServiceSettings serviceSettings;
	serviceSettings.callTimeout = conf->sendReceiveTimeout * 1000;
	UaDataValues values;
	UaDiagnosticInfos diagnosticInfos;
	log->debug("Reading %d node attributes...", attrToReadIndex);
	UaStatus result = session->read(serviceSettings, 0 /*maxAge*/,
			OpcUa_TimestampsToReturn_Both, attrToRead, values, diagnosticInfos);
	// if reading failed
	if (!result.isGood()) {
		// remove added node attributes
		for (int i = 0; i < returnNodeAttributes.size(); i++) {
			delete returnNodeAttributes[i];
		}
		returnNodeAttributes.clear();
		// throw an exception
		std::ostringstream msg;
		msg << "Cannot read values: " << result.toString().toUtf8();
		throw ExceptionDef(HaSessionException, msg.str());
	}
	log->debug("Read %d node attributes", values.length());
	// for each result
	for (OpcUa_UInt32 nodesToReadIndex = 0;
			nodesToReadIndex < attrToRead.length(); nodesToReadIndex++) {
		UaNodeId nodeId(attrToRead[nodesToReadIndex].NodeId);
		UaStatus status(values[nodesToReadIndex].StatusCode);
		OpcUa_UInt32 retIndex = indexMap[nodesToReadIndex];
		if (status.isGood()) {
			if (log->isDebugEnabled()) {
				log->debug("Read %s attributeId=%d,value=%s",
						nodeId.toXmlString().toUtf8(),
						attrToRead[nodesToReadIndex].AttributeId,
						UaVariant(values[nodesToReadIndex].Value).toFullString().toUtf8());
			}
			switch (attrToRead[nodesToReadIndex].AttributeId) {
			case OpcUa_Attributes_NodeClass: {
				UaVariant value(values[nodesToReadIndex].Value);
				OpcUa_NodeClass* nodeClass = new OpcUa_NodeClass;
				value.toInt32(*(OpcUa_Int32*) nodeClass);
				returnNodeAttributes[retIndex]->setNodeClass(nodeClass);
				break;
			}
			case OpcUa_Attributes_Value:
				returnNodeAttributes[retIndex]->setValue(
						new UaVariant(values[nodesToReadIndex].Value));
				break;
			case OpcUa_Attributes_DataType: {
				UaVariant value(values[nodesToReadIndex].Value);
				UaNodeId* dataTypeId = new UaNodeId();
				value.toNodeId(*dataTypeId);
				returnNodeAttributes[retIndex]->setDataType(dataTypeId);
				break;
			}
			}
		} else {
			if (log->isDebugEnabled()) {
				log->debug("Read %s attributeId=%d,status=%s",
						nodeId.toXmlString().toUtf8(),
						attrToRead[nodesToReadIndex].AttributeId,
						status.toString().toUtf8());
			}
			std::ostringstream msg;
			msg << "Cannot read value for " << nodeId.toXmlString().toUtf8()
					<< " and attributeId "
					<< attrToRead[nodesToReadIndex].AttributeId << ": "
					<< status.toString().toUtf8();
			returnNodeAttributes[retIndex]->setException(
					new ExceptionDef(HaSessionException, msg.str()));
		}
	}
}

UaVariant HaSession::getVariable(const UaNodeId &variableNodeId){
	UaClientSdk::ServiceSettings serviceSettings;
	serviceSettings.callTimeout = d->conf->sendReceiveTimeout * 1000;
	UaClientSdk::BrowseContext browseContext;
	browseContext.browseDirection = OpcUa_BrowseDirection_Both;
	browseContext.referenceTypeId = OpcUaId_HierarchicalReferences;
	browseContext.maxReferencesToReturn = 1;
	browseContext.nodeClassMask = OpcUa_NodeClass_Variable;
	UaByteString continuationPoint;
	UaReferenceDescriptions referenceDescriptions;
	UaStatus result = d->session->browse(serviceSettings, variableNodeId,
			browseContext, continuationPoint, referenceDescriptions);
	if (referenceDescriptions.length() == 1){
		return UaVariant(referenceDescriptions[0].NodeId.NodeId);
	}
	return UaVariant(false);

}

void HaSessionPrivate::getEventFields(const UaNodeId & eventTypeId,
		std::vector<BinaryServerNamespace::EventField*>& returnEventFields)
		/*throws HaSessionException*/{
	UaClientSdk::ServiceSettings serviceSettings;
	serviceSettings.callTimeout = conf->sendReceiveTimeout * 1000;

	UaClientSdk::BrowseContext browseContext;
	browseContext.browseDirection = OpcUa_BrowseDirection_Both;
	browseContext.referenceTypeId = OpcUaId_HierarchicalReferences;
	browseContext.includeSubtype = OpcUa_True;
	browseContext.maxReferencesToReturn = 0;
	browseContext.nodeClassMask = OpcUa_NodeClass_Variable
			| OpcUa_NodeClass_ObjectType;

	UaByteString continuationPoint;
	UaReferenceDescriptions referenceDescriptions;
	UaStatus result = session->browse(serviceSettings, eventTypeId,
			browseContext, continuationPoint, referenceDescriptions);
	if (!result.isGood()) {
		std::ostringstream msg;
		msg << "Cannot get event fields for "
				<< eventTypeId.toXmlString().toUtf8() << ": "
				<< result.toString().toUtf8();
		throw ExceptionDef(HaSessionException, msg.str());
	}
	// add variables
	for (OpcUa_UInt32 i = 0; i < referenceDescriptions.length(); i++) {
		// if variable
		if (OpcUaId_HasSubtype
				!= UaNodeId(referenceDescriptions[i].ReferenceTypeId).identifierNumeric()) {
			// check for duplicate browse name (eg. overridden variable)
			bool found = false;
			for (int j = 0; j < returnEventFields.size() && !found; j++) {
				found = (*returnEventFields[j]->getQualifiedName())
						== UaQualifiedName(referenceDescriptions[i].BrowseName);
			}
			if (found) {
				// skip variable
				continue;
			}
			// read dataTypeId of event field
			UaNodeId nodeId(referenceDescriptions[i].NodeId.NodeId);
			std::vector<const UaNodeId*> nodeIds;
			nodeIds.push_back(&nodeId);
			std::vector<NodeAttributes*>* nodeAttributes = new std::vector<
					NodeAttributes*>();
			VectorScopeGuard<NodeAttributes> nodeAttributesSG(nodeAttributes);
			BinaryServerNamespace::EventField* field =
					new BinaryServerNamespace::EventField(*new UaNodeId(nodeId),
							true /*attachValues*/);
			try {
				parent->read(nodeIds, *nodeAttributes); // HaSessionException
				// if the data type could be read
				if (nodeAttributes != NULL && nodeAttributes->size() > 0) {
					// if exception is returned
					if ((*nodeAttributes)[0]->getException() != NULL) {
						std::ostringstream msg;
						msg << "Cannot get data type of event field "
								<< nodeId.toXmlString().toUtf8()
								<< " of event type "
								<< eventTypeId.toXmlString().toUtf8();
						HaSessionException* exception =
								new ExceptionDef(HaSessionException, msg.str());
						exception->setCause(
								(*nodeAttributes)[0]->getException());
						field->setException(exception);
					} else {
						field->setQualifiedName(
								new UaQualifiedName(
										referenceDescriptions[i].BrowseName));
						field->setDataTypeId(
								new UaNodeId(
										*(*nodeAttributes)[0]->getDataType()));
					}
				} else {
					std::ostringstream msg;
					msg << "Cannot read node attributes of event field "
							<< nodeId.toXmlString().toUtf8()
							<< " of event type "
							<< eventTypeId.toXmlString().toUtf8();
					field->setException(
							new ExceptionDef(HaSessionException, msg.str()));
				}
			} catch (Exception& e) {
				std::ostringstream msg;
				msg << "Cannot get data type of event field "
						<< nodeId.toXmlString().toUtf8() << " of event type "
						<< eventTypeId.toXmlString().toUtf8();
				HaSessionException* exception =
						new ExceptionDef(HaSessionException, msg.str());
				exception->setCause(&e);
				field->setException(exception);
			}
			returnEventFields.push_back(field);
		}
	}
	// add variables of super types
	for (OpcUa_UInt32 i = 0; i < referenceDescriptions.length(); i++) {
		UaNodeId nodeId(referenceDescriptions[i].NodeId.NodeId);
		try {
			// if super type or sub type
			if (OpcUaId_HasSubtype
					== UaNodeId(referenceDescriptions[i].ReferenceTypeId).identifierNumeric()) {
				// if super type
				if (!referenceDescriptions[i].IsForward) {
					// get event fields of super type
					getEventFields(nodeId, returnEventFields); // HaSessionException
				}
				// childs are ignored
			}
		} catch (Exception& e) {
			// process as many event fields as possible
			std::string st;
			e.getStackTrace(st);
			log->error("Exception while getting event fields: %s", st.c_str());
		}
	}
}

void HaSessionPrivate::connectionStatusChanged(OpcUa_UInt32 clientConnectionId,
		UaClientSdk::UaClient::ServerStatus serverStatus) {

	callback->connectionStateChanged(serverStatus);

	switch (serverStatus) {
	case UaClientSdk::UaClient::Disconnected:
		log->debug("Connection status changed to Disconnected");
		break;
	case UaClientSdk::UaClient::Connected:
		log->debug("Connection status changed to Connected");
		break;
	case UaClientSdk::UaClient::ConnectionWarningWatchdogTimeout:
		log->debug(
				"Connection status changed to ConnectionWarningWatchdogTimeout");
		break;
	case UaClientSdk::UaClient::ConnectionErrorApiReconnect:
		log->debug("Connection status changed to ConnectionErrorApiReconnect");
		break;
	case UaClientSdk::UaClient::ServerShutdown:
		log->debug("Connection status changed to ServerShutdown");
		break;
	case UaClientSdk::UaClient::NewSessionCreated:
		log->debug("Connection status changed to NewSessionCreated");
		break;
	}
}
