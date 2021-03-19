#include "JDataProvider.h"
#include "../messages/ConverterBin2IO.h"
#include "../messages/dto/Call.h"
#include "../messages/dto/CallResponse.h"
#include "../messages/dto/Event.h"
#include "../messages/dto/MessageHeader.h"
#include "../messages/dto/Notification.h"
#include "../messages/dto/Read.h"
#include "../messages/dto/ReadResponse.h"
#include "../messages/dto/Scalar.h"
#include "../messages/dto/Subscribe.h"
#include "../messages/dto/SubscribeResponse.h"
#include "../messages/dto/Unsubscribe.h"
#include "../messages/dto/UnsubscribeResponse.h"
#include "../messages/dto/Write.h"
#include "../messages/dto/WriteResponse.h"
#include <common/Exception.h>
#include <common/Mutex.h>
#include <common/MutexLock.h>
#include <common/ScopeGuard.h>
#include <common/VectorScopeGuard.h>
#include <common/logging/Logger.h>
#include <common/logging/LoggerFactory.h>
#include <ioDataProvider/IODataProviderException.h>
#include <ioDataProvider/OpcUaEventData.h>
#include <pthread.h> // pthread_t
#include <sstream> // std::ostringstream
#include <string>
#include <time.h> // nanosleep
#include <ctime> // std::time_t

#include <uastructuredefinition.h>
#include <uaargument.h>


#ifdef DEBUG
#include <CppUTest/MemoryLeakDetectorNewMacros.h>
#endif

using namespace CommonNamespace;

class JDataProviderPrivate {
	friend class JDataProvider;

private:
	class CallbackData {
	public:
		const IODataProviderNamespace::NodeId* nodeId;
		IODataProviderNamespace::SubscriberCallback* callback;
	};

	Logger* log;

	ConverterBin2IO converter;

	bool unitTesting;
	int namespaceIndex;
	unsigned long messageIdCounter;

	Mutex* mutex;

	std::vector<CallbackData*> callbacks;

};

JDataProvider::JDataProvider(bool unitTesting) /* throws MutexException */{
	d = new JDataProviderPrivate();
	d->log = LoggerFactory::getLogger("JDataProvider");
	d->mutex = new Mutex(); // MutexException
	nodeBrowser = NULL;
	jvm = NULL;
	native2j = NULL;
	jDataProvider = NULL;
}

JDataProvider::~JDataProvider() {
	if (native2j != NULL) {
		delete native2j;
	}
	close();
	delete d->mutex;
	delete d;
}

void JDataProvider::open(
		const std::string& confDir) /* throws IODataProviderException */{
	std::ostringstream msg;
	msg << "This provider is for JNI usage only";
	IODataProviderNamespace::IODataProviderException ex = ExceptionDef(
			IODataProviderNamespace::IODataProviderException, msg.str());
	throw ex;
}

void JDataProvider::open(JNIEnv *env, jobject properties,
		jobject dataProvider) /* throws IODataProviderException */{
	native2j = new Native2J(env, NULL);
	jDataProvider = env->NewGlobalRef(dataProvider);
	env->GetJavaVM(&jvm);
}

void JDataProvider::setNodeBrowser(SASModelProviderNamespace::NodeBrowser* nodeBrowser){
	this->nodeBrowser = nodeBrowser;
}

void JDataProvider::close() {
	{
		MutexLock lock(*d->mutex);
	}
}


bool JDataProvider::findFieldModel(const UaNodeId &start) {

	bool changed = false;
	if (nodeBrowser == NULL || fields.count(start.toFullString().toUtf8())){
		return changed;
	}
	UaStructureDefinition def = nodeBrowser->getStructureDefinition(start);
	std::map<std::string, ModelType> values;
	if (def.childrenCount() > 0){
		for (int j = 0; j < def.childrenCount(); j++) {
			UaStructureField sf = def.child(j);
			// if union field for value
			ModelType t;
			if (sf.typeId().namespaceIndex() != 0){
				t.type = ModelType::REF;
				if (sf.typeId().identifierType() == OpcUa_IdentifierType_Numeric){
					t.ref = ParamId(sf.typeId().namespaceIndex(), sf.typeId().identifierNumeric()).toString();
				} else {
					t.ref = ParamId(sf.typeId().namespaceIndex(), UaString(sf.typeId().identifierString()).toUtf8()).toString();
				}
				findFieldModel(sf.typeId());
			} else {
				t.type = ModelType::TYPE;
				t.t = sf.typeId().identifierNumeric();
			}
			values[sf.name().toUtf8()] = t;
		}
	} else {
		//Child count 0
		std::vector<UaNodeId>* superTypes = nodeBrowser->getSuperTypes(start); // ConversionException
		ScopeGuard<std::vector<UaNodeId> > sSuperTypes(superTypes);
		ModelType t;
		if (superTypes->size() > 0) {

			if (superTypes->back().namespaceIndex() != 0) {
				t.type = ModelType::REF;
				if (superTypes->back().identifierType()
						== OpcUa_IdentifierType_Numeric) {
					t.ref = ParamId(superTypes->back().namespaceIndex(),
							superTypes->back().identifierNumeric()).toString();
				} else {
					t.ref =
							ParamId(superTypes->back().namespaceIndex(),
									UaString(
											superTypes->back().identifierString()).toUtf8()).toString();
				}
				findFieldModel(superTypes->back());
			} else {
				t.type = ModelType::TYPE;
				t.t = superTypes->back().identifierNumeric();
			}

		} else {
			t.type = ModelType::TYPE;
			if (start.identifierType() == OpcUa_IdentifierType_Numeric) {
				t.t = start.identifierNumeric();
			}
		}
		values[start.toFullString().toUtf8()] = t;

	}
	fields[start.toFullString().toUtf8()] = values;
	changed = true;
	return changed;
}

const IODataProviderNamespace::NodeProperties* JDataProvider::getDefaultNodeProperties(
		const std::string& namespaceUri, int namespaceIndex) {
	// the interface to the server part does not support namespaces => use the last one
	d->namespaceIndex = namespaceIndex;
	//TODO get supported namespaces and their value handling from conf
	return new IODataProviderNamespace::NodeProperties(
			IODataProviderNamespace::NodeProperties::ASYNC);
}

std::vector<const IODataProviderNamespace::NodeData*>* JDataProvider::getNodeProperties(
		const std::string& namespaceUri, int namespaceIndex) {
	//TODO get nodes data from conf
	std::vector<const IODataProviderNamespace::NodeData*>* ret =
			new std::vector<const IODataProviderNamespace::NodeData*>();
	int idsNoneCount = 12;
	const char* idsNone[idsNoneCount] = { "AirConditioner_1.Humidity.EURange",
			"AirConditioner_1.Humidity.EngineeringUnits",
			"AirConditioner_1.HumiditySetpoint.EURange",
			"AirConditioner_1.HumiditySetpoint.EngineeringUnits",
			"AirConditioner_1.Temperature.EURange",
			"AirConditioner_1.Temperature.EngineeringUnits",
			"AirConditioner_1.TemperatureSetpoint.EURange",
			"AirConditioner_1.TemperatureSetpoint.EngineeringUnits",
			"Furnace_2.Temperature.EURange",
			"Furnace_2.Temperature.EngineeringUnits",
			"Furnace_2.TemperatureSetpoint.EURange",
			"Furnace_2.TemperatureSetpoint.EngineeringUnits" };
	for (int i = 0; i < idsNoneCount; i++) {
		ret->push_back(
				new IODataProviderNamespace::NodeData(
						*new IODataProviderNamespace::NodeId(namespaceIndex,
								*new std::string(idsNone[i]),
								true /* attachValues */),
						new IODataProviderNamespace::NodeProperties(
								IODataProviderNamespace::NodeProperties::NONE),
						true /* attachValues */));
	}

	int idsSyncCount = 20;
	const char* idsSync[idsSyncCount] = { "rfr310.IOData.HS1",
			"rfr310.IOData.HS1.direction", "rfr310.IOData.HS2",
			"rfr310.IOData.HS2.direction", "rfr310.IOData.HS3",
			"rfr310.IOData.HS3.direction", "rfr310.IOData.HS4",
			"rfr310.IOData.HS4.direction", "rfr310.IOData.SWS1_SWD1",
			"rfr310.IOData.SWS1_SWD1.direction", "rfr310.IOData.SWS2_SWD2",
			"rfr310.IOData.SWS2_SWD2.direction", "rfr310.IOData.LS1",
			"rfr310.IOData.LS1.direction", "rfr310.IOData.LS2",
			"rfr310.IOData.LS2.direction", "rfr310.RuntimeParameters.RfPower",
			"rfr310.RuntimeParameters.MinRssi", "AirConditioner_1.Temperature",
			"ControllerConfigurations" };
	for (int i = 0; i < idsSyncCount; i++) {
		ret->push_back(
				new IODataProviderNamespace::NodeData(
						*new IODataProviderNamespace::NodeId(namespaceIndex,
								*new std::string(idsSync[i]),
								true /* attachValues */),
						new IODataProviderNamespace::NodeProperties(
								IODataProviderNamespace::NodeProperties::SYNC),
						true /* attachValues */));
	}
	return ret;
}

UaNodeId JDataProvider::getUaNode(ParamId *pId) {
	if (pId->getParamIdType() == ParamId::STRING){
		return UaNodeId(UaString(pId->getString().c_str()), pId->getNamespaceIndex());
	} else {
		return UaNodeId(pId->getNumeric(), pId->getNamespaceIndex());
	}
}

std::string JDataProvider::getParamId(UaNodeId nId){
	UaVariable *uaVar = nodeBrowser->getVariable(nId);
	if (uaVar != NULL){
		UaNodeId dataTypeId = uaVar->dataType();
		if (dataTypeId.identifierType() == OpcUa_IdentifierType_Numeric){
			return ParamId(dataTypeId.namespaceIndex(), dataTypeId.identifierNumeric()).toString();
		} else if (nId.identifierType() == OpcUa_IdentifierType_String){
			return ParamId(dataTypeId.namespaceIndex(), UaString(dataTypeId.identifierString()).toUtf8()).toString();
		}
	}
	return std::string();
}

void JDataProvider::updateModel(UaNodeId nId){
	bool changed = false;
	UaVariable* uaVar = nodeBrowser->getVariable(nId);
	if (uaVar != NULL){
		changed = findFieldModel(nodeBrowser->getVariable(nId)->dataType());
		if (changed){
			native2j->updateModel(fields);
		}
	}
}

std::vector<IODataProviderNamespace::NodeData*>* JDataProvider::read(
		const std::vector<const IODataProviderNamespace::NodeId*>& nodeIds) /* throws IODataProviderException */{
	unsigned long messageId;
	int sendReceiveTimeout;
	int namespaceIndex;
	{
		MutexLock lock(*d->mutex);
		messageId = d->messageIdCounter++;
		namespaceIndex = d->namespaceIndex;
	}
	std::vector<IODataProviderNamespace::NodeData*>* ret = new std::vector<
			IODataProviderNamespace::NodeData*>();
	VectorScopeGuard<IODataProviderNamespace::NodeData> retSG(ret);
	// for each node
	for (int i = 0; i < nodeIds.size(); i++) {
		const IODataProviderNamespace::NodeId& nodeId = *nodeIds[i];
		IODataProviderNamespace::Variant* nodeValue = NULL;
		IODataProviderNamespace::IODataProviderException* exception = NULL;
		try {
			ParamId* paramId = d->converter.convertIo2bin(nodeId); // ConversionException
			ScopeGuard<ParamId> sParamId(paramId);
			UaNodeId uaNode = getUaNode(paramId);

			updateModel(uaNode);

			JNIEnv *tmpEnv;
			jvm->AttachCurrentThread((void **) &tmpEnv, NULL);
			jclass havis_util_opcua_DataProvider = tmpEnv->FindClass(
					"havis/util/opcua/DataProvider");

			jmethodID havis_util_opcua_DataProvider_read = tmpEnv->GetMethodID(
					havis_util_opcua_DataProvider, "read",
					"(ILjava/lang/Object;)Ljava/lang/Object;");
			jstring node = tmpEnv->NewStringUTF(paramId->toString().c_str());
			if (paramId->getParamIdType() == ParamId::STRING) {
				node = tmpEnv->NewStringUTF(paramId->getString().c_str());
			}
			jobject result = tmpEnv->CallObjectMethod(jDataProvider,
					havis_util_opcua_DataProvider_read, namespaceIndex, node);

			ReadResponse* readResponse = new ReadResponse(999, Status::SUCCESS);
			ScopeGuard<ReadResponse> readResponseSG(readResponse);
			if (readResponse->getStatus() == Status::SUCCESS) {
				// convert param value
				if (result != NULL) {
					ModelType t;
					t.type = ModelType::REF;
					t.ref = getParamId(uaNode);
					if (t.ref.length()>0){
						Variant* res = native2j->getVariant(tmpEnv, result, t.ref, t);
						ScopeGuard<Variant> sRes(res);
						nodeValue = d->converter.convertBin2io(
								*res, namespaceIndex);
					}
				}
			} else {
				std::ostringstream msg;
				msg << "Received a read response for "
						<< nodeId.toString().c_str() << ": messageId="
						<< messageId << ",status=" << readResponse->getStatus();
				exception =
						new ExceptionDef(IODataProviderNamespace::IODataProviderException,
								msg.str());
			}
			jvm->DetachCurrentThread();
		} catch (Exception& e) {
			exception =
					new ExceptionDef(IODataProviderNamespace::IODataProviderException,
							std::string("Cannot read data for ").append(nodeId.toString()));
			exception->setCause(&e);
		}
		// add value to result list
		IODataProviderNamespace::NodeData* nodeData =
				new IODataProviderNamespace::NodeData(
						*new IODataProviderNamespace::NodeId(nodeId), nodeValue,
						true /* attachValues */);
		nodeData->setException(exception);
		ret->push_back(nodeData);
		MutexLock lock(*d->mutex);
		messageId = d->messageIdCounter++;
	}
	return retSG.detach();
}

void JDataProvider::write(
		const std::vector<const IODataProviderNamespace::NodeData*>& nodeData,
		bool sendValueChangedEvents) /* throws IODataProviderException */{
	unsigned long messageId;
	int sendReceiveTimeout;
	{
		MutexLock lock(*d->mutex);
		messageId = d->messageIdCounter++;
	}
	IODataProviderNamespace::IODataProviderException* exception = NULL;
	for (int i = 0; i < nodeData.size(); i++) {
		const IODataProviderNamespace::NodeData& data = *nodeData[i];
		// create paramId
		const IODataProviderNamespace::NodeId& nodeId = data.getNodeId();
		try {
			ParamId* paramId = d->converter.convertIo2bin(nodeId); // ConversionException
			ScopeGuard<ParamId> paramIdSG(paramId);
			// create paramValue
			const IODataProviderNamespace::Variant* nodeValue = data.getData();
			if (nodeValue == NULL && exception == NULL) {
				exception =
						new ExceptionDef(IODataProviderNamespace::IODataProviderException,
								std::string("Cannot write a NULL value for ").append(nodeId.toString()));
				// continue with next node
				continue;
			}

			// send request
			// get response
			JNIEnv *tmpEnv;
			int ret = jvm->AttachCurrentThread((void **) &tmpEnv, NULL);

			jclass havis_util_opcua_DataProvider = tmpEnv->FindClass(
					"havis/util/opcua/DataProvider");

			if (havis_util_opcua_DataProvider == NULL) {
				return;
			}

			jmethodID havis_util_opcua_DataProvider_write = tmpEnv->GetMethodID(
					havis_util_opcua_DataProvider, "write",
					"(ILjava/lang/Object;Ljava/lang/Object;)V");
			jstring node = tmpEnv->NewStringUTF(paramId->toString().c_str());
			if (paramId->getParamIdType() == ParamId::STRING) {
				node = tmpEnv->NewStringUTF(paramId->getString().c_str());
			}

			Variant* paramValue = d->converter.convertIo2bin(*nodeValue); // ConversionException
			// create request
			Write request(messageId, *paramIdSG.detach(), *paramValue,
					true /* attachValues */);
			// send request
			// get response

			Status::Value result = Status::SUCCESS;

			tmpEnv->CallVoidMethod(jDataProvider,
					havis_util_opcua_DataProvider_write, paramId->getNamespaceIndex(), node,
					native2j->getVariant(tmpEnv, *paramValue));
			//            if (tmpEnv->ExceptionCheck()){
			//            	result = Status::APPLICATION_ERROR;
			//            	d->log->info("Hello there.");
			//            }

			WriteResponse* writeResponse = new WriteResponse(999, result); // TimeoutException
			ScopeGuard<WriteResponse> writeResponseSG(writeResponse);

			jvm->DetachCurrentThread();

			if (writeResponse->getStatus()
					!= Status::SUCCESS&& exception == NULL) {
				std::ostringstream msg;
				msg << "Received a write response for "
						<< nodeId.toString().c_str() << ": messageId="
						<< messageId << ",status="
						<< writeResponse->getStatus();
				exception =
						new ExceptionDef(IODataProviderNamespace::IODataProviderException,
								msg.str());
			}
			MutexLock lock(*d->mutex);
			messageId = d->messageIdCounter++;
		} catch (Exception& e) {
			if (exception == NULL) {
				exception =
						new ExceptionDef(IODataProviderNamespace::IODataProviderException,
								std::string("Cannot write data for nodeId ").append(nodeId.toString()));
				exception->setCause(&e);
			}
		}
	}
	if (exception != NULL) {
		IODataProviderNamespace::IODataProviderException ex = ExceptionDef(
				IODataProviderNamespace::IODataProviderException,
				std::string("Cannot write data"));
		ex.setCause(exception);
		delete exception;
		throw ex;
	}
}

std::vector<IODataProviderNamespace::MethodData*>* JDataProvider::call(
		const std::vector<const IODataProviderNamespace::MethodData*>& methodData) {
	std::vector<IODataProviderNamespace::MethodData*>* ret = new std::vector<
			IODataProviderNamespace::MethodData*>();
	VectorScopeGuard<IODataProviderNamespace::MethodData> retSG(ret);
	unsigned long messageId;
	int sendReceiveTimeout;
	int namespaceIndex;
	{
		MutexLock lock(*d->mutex);
		messageId = d->messageIdCounter++;
		namespaceIndex = d->namespaceIndex;
	}
	// for each method
	for (int i = 0; i < methodData.size(); i++) {
		const IODataProviderNamespace::MethodData& methodDataElem =
				*methodData[i];
		std::vector<const IODataProviderNamespace::Variant*>* outputArgs =
				new std::vector<const IODataProviderNamespace::Variant*>();
		VectorScopeGuard<const IODataProviderNamespace::Variant> outputArgsSG(
				outputArgs);
		IODataProviderNamespace::IODataProviderException* exception = NULL;

		Exception* cEx = NULL;

		try {
            ParamId* methodId = d->converter.convertIo2bin(
                    methodDataElem.getMethodNodeId()); // ConversionException
            ScopeGuard<ParamId> methodIdSG(methodId);
            ParamId* objectId = d->converter.convertIo2bin(
                               methodDataElem.getObjectNodeId()); // ConversionException
            ScopeGuard<ParamId> objectIdSG(objectId);


            std::map<std::string, std::map<std::string, ModelType> > fields;
            UaNodeId uaNodeId(UaString(methodId->getString().c_str()), methodId->getNamespaceIndex());
            UaMethod *test = nodeBrowser->getMethod(uaNodeId);
            UaVariable* argsVariable = static_cast<UaVariable*> (
                            test->getUaReferenceLists()->getTargetNodeByBrowseName(
                            UaQualifiedName("OutputArguments", 0 /*nsIndex*/)));

            if (argsVariable != NULL){
            	UaVariant args = UaVariant(*argsVariable->value(NULL /* session*/).value());
				UaExtensionObjectArray argsEOA;
				args.toExtensionObjectArray(argsEOA);
				for (int i = 0; i < argsEOA.length(); i++) {
					UaArgument *a = new UaArgument(argsEOA[i]);
					bool changed = findFieldModel(a->getDataType());
					if (changed) {
						native2j->updateModel(fields);
					}
				}
            }

			// send request
			// get response
			JNIEnv *tmpEnv;
			jvm->AttachCurrentThread((void **) &tmpEnv, NULL);
			jclass havis_util_opcua_DataProvider = tmpEnv->FindClass(
					"havis/util/opcua/DataProvider");

			jmethodID havis_util_opcua_DataProvider_exec = tmpEnv->GetMethodID(
					havis_util_opcua_DataProvider, "exec",
					"(ILjava/lang/Object;ILjava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");

			jclass java_lang_Object = tmpEnv->FindClass("java/lang/Object");
			jobjectArray params = tmpEnv->NewObjectArray(methodDataElem.getMethodArguments().size(), java_lang_Object,
				NULL);
			jstring node = tmpEnv->NewStringUTF(objectId->getString().c_str());
			jstring method = tmpEnv->NewStringUTF(methodId->getString().c_str());

			for (int j = 0; j < methodDataElem.getMethodArguments().size(); j++) {
				Variant *value = d->converter.convertIo2bin(
						*methodDataElem.getMethodArguments()[j]); // ConversionException
				tmpEnv->SetObjectArrayElement(params, j,
						native2j->getVariant(tmpEnv, *value));
			}
			jobject result = tmpEnv->CallObjectMethod(jDataProvider, havis_util_opcua_DataProvider_exec, namespaceIndex, method, namespaceIndex, node, params);

			CallResponse* callResponse = new CallResponse(messageId, Status::SUCCESS); // TimeoutException
			ScopeGuard<CallResponse> callResponseSG(callResponse);

			cEx = native2j->getException(tmpEnv);
			if (cEx != NULL){
				callResponse->setStatus(Status::APPLICATION_ERROR);
			}
			switch (callResponse->getStatus()) {
			case Status::SUCCESS: {
				if (result != NULL){
					Array *varArr = (Array*)native2j->getVariant(tmpEnv, result);
					const std::vector<const Variant*>& elements =
							varArr->getElements();
					// for each method output argument
					for (int j = 0; j < elements.size(); j++) {
						// convert Variant (binary) to Variant (IODataProvider)
						IODataProviderNamespace::Variant* value = d->converter.convertBin2io(
								 *(elements.at(j)), namespaceIndex); // ConversionException
						// add value to output arguments
						outputArgs->push_back(value);
					}
				}
				break;
			}
			case Status::APPLICATION_ERROR: {
				std::ostringstream msg;
				msg << "Received a call response with application error " << std::hex << cEx->getErrorCode() << ":" << cEx->getMessage();
				exception =
						new ExceptionDef(IODataProviderNamespace::IODataProviderException,
								msg.str());
				unsigned long *errorCode = new unsigned long (*cEx->getErrorCode());
				exception->setErrorCode(errorCode);
				d->log->error("Failed to execute: %s", msg.str().c_str());
				break;
			}
			default:
				std::ostringstream msg;
				msg << "Received a call response with status "
						<< callResponse->getStatus();
				exception =
						new ExceptionDef(IODataProviderNamespace::IODataProviderException,
								msg.str());
			}
			jvm->DetachCurrentThread();
		} catch (Exception& e) {
			exception =
					new ExceptionDef(IODataProviderNamespace::IODataProviderException,
							std::string("Cannot call method ")
							.append(methodDataElem.getMethodNodeId().toString())
							.append(" on object ")
							.append(methodDataElem.getObjectNodeId().toString()));
			exception->setCause(&e);
		}
		IODataProviderNamespace::MethodData* md =
				new IODataProviderNamespace::MethodData(
						*new IODataProviderNamespace::NodeId(
								methodDataElem.getObjectNodeId()),
						*new IODataProviderNamespace::NodeId(
								methodDataElem.getMethodNodeId()),
						*outputArgsSG.detach(), false /*attachValues*/);
		md->setException(exception);
		ret->push_back(md);
		MutexLock lock(*d->mutex);
		messageId = d->messageIdCounter++;
	}
	return retSG.detach();
}

std::vector<IODataProviderNamespace::NodeData*>* JDataProvider::subscribe(
		const std::vector<const IODataProviderNamespace::NodeId*>& nodeIds,
		IODataProviderNamespace::SubscriberCallback& callback) /* throws IODataProviderException */{
	unsigned long messageId;
	int sendReceiveTimeout;
	{
		MutexLock lock(*d->mutex);
		messageId = d->messageIdCounter++;
	}
	std::vector<IODataProviderNamespace::IODataProviderException*> exceptions;
	// for each node
	for (int i = 0; i < nodeIds.size(); i++) {
		const IODataProviderNamespace::NodeId& nodeId = *nodeIds[i];
		//TODO throw exception if already subscribed
		IODataProviderNamespace::IODataProviderException* exception = NULL;
		try {
			// create paramId
			ParamId* paramId = d->converter.convertIo2bin(nodeId); // ConversionException
			// create request
			Subscribe request(messageId, *paramId, true /* attachValues */);
			// send request
			// get response

			JNIEnv *tmpEnv;
			jvm->AttachCurrentThread((void **) &tmpEnv, NULL);
			jclass havis_util_opcua_DataProvider = tmpEnv->FindClass(
					"havis/util/opcua/DataProvider");

			jmethodID havis_util_opcua_DataProvider_subscribe =
					tmpEnv->GetMethodID(havis_util_opcua_DataProvider,
							"subscribe", "(ILjava/lang/Object;)V");
			jstring node = tmpEnv->NewStringUTF(paramId->toString().c_str());
			if (paramId->getParamIdType() == ParamId::STRING) {
				node = tmpEnv->NewStringUTF(paramId->getString().c_str());
			} else {
				stringstream ss;
				ss << paramId->getNumeric();
				node = tmpEnv->NewStringUTF(ss.str().c_str());
			}

			tmpEnv->CallVoidMethod(jDataProvider,
					havis_util_opcua_DataProvider_subscribe, paramId->getNamespaceIndex(), node);
			SubscribeResponse* subscribeResponse = new SubscribeResponse(999,
					Status::SUCCESS); // TimeoutException
			ScopeGuard<SubscribeResponse> subscribeResponseSG(
					subscribeResponse);
			if (subscribeResponse->getStatus() != Status::SUCCESS) {
				std::ostringstream msg;
				msg << "Received a subscribe response for "
						<< nodeId.toString().c_str() << ": messageId="
						<< messageId << ",status="
						<< subscribeResponse->getStatus();
				exception =
						new ExceptionDef(IODataProviderNamespace::IODataProviderException,
								msg.str());
			}
		} catch (Exception& e) {
			exception =
					new ExceptionDef(IODataProviderNamespace::IODataProviderException,
							std::string("Cannot create a subscription for ").append(nodeId.toString()));
			exception->setCause(&e);
		}
		exceptions.push_back(exception);
		MutexLock lock(*d->mutex);
		if (exception == NULL) {
			// add callback to list
			JDataProviderPrivate::CallbackData* callbackData =
					new JDataProviderPrivate::CallbackData();
			callbackData->nodeId = new IODataProviderNamespace::NodeId(nodeId);
			callbackData->callback = &callback;
			d->callbacks.push_back(callbackData);
		}
		// get new messageId
		messageId = d->messageIdCounter++;
	}
	// set exceptions to read results
	std::vector<IODataProviderNamespace::NodeData*>* readResults = read(
			nodeIds);
	for (int i = 0; i < exceptions.size(); i++) {
		IODataProviderNamespace::IODataProviderException* exception =
				exceptions[i];
		if (exception != NULL) {
			(*readResults)[i]->setException(exception);
		}
	}
	return readResults;
}

void JDataProvider::unsubscribe(
		const std::vector<const IODataProviderNamespace::NodeId*>& nodeIds) /* throws IODataProviderException */{
	unsigned long messageId;
	int sendReceiveTimeout;
	{
		MutexLock lock(*d->mutex);
		messageId = d->messageIdCounter++;
	}
	IODataProviderNamespace::IODataProviderException* exception = NULL;
	// for each node in reverse order
	for (int i = nodeIds.size() - 1; i >= 0; i--) {
		const IODataProviderNamespace::NodeId& nodeId = *nodeIds[i];
		try {
			// create paramId
			ParamId* paramId = d->converter.convertIo2bin(nodeId); // ConversionException
			// create request
			Unsubscribe request(messageId, *paramId, true /* attachValues */);
			// send request
			// get response

			JNIEnv *tmpEnv;
			jvm->AttachCurrentThread((void **) &tmpEnv, NULL);
			jclass havis_util_opcua_DataProvider = tmpEnv->FindClass(
					"havis/util/opcua/DataProvider");

			jmethodID havis_util_opcua_DataProvider_unsubscribe =
					tmpEnv->GetMethodID(havis_util_opcua_DataProvider,
							"unsubscribe", "(ILjava/lang/Object;)V");
			jstring node = tmpEnv->NewStringUTF(paramId->toString().c_str());
			if (paramId->getParamIdType() == ParamId::STRING) {
				node = tmpEnv->NewStringUTF(paramId->getString().c_str());
			} else {
				stringstream ss;
				ss << paramId->getNumeric();
				node = tmpEnv->NewStringUTF(ss.str().c_str());
			}

			tmpEnv->CallVoidMethod(jDataProvider,
					havis_util_opcua_DataProvider_unsubscribe, paramId->getNamespaceIndex(), node);
			UnsubscribeResponse* unsubscribeResponse = new UnsubscribeResponse(999,
					Status::SUCCESS); // TimeoutException
			ScopeGuard<UnsubscribeResponse> unsubscribeResponseSG(
					unsubscribeResponse);

			if (unsubscribeResponse->getStatus()
					!= Status::SUCCESS&& exception == NULL) {
				std::ostringstream msg;
				msg << "Received an unsubscribe response for "
						<< nodeId.toString().c_str() << ": messageId="
						<< messageId << ",status="
						<< unsubscribeResponse->getStatus();
				exception =
						new ExceptionDef(IODataProviderNamespace::IODataProviderException,
								msg.str());
			}
		} catch (Exception& e) {
			if (exception == NULL) {
				// convert exception to IODataProviderException
				exception =
						new ExceptionDef(IODataProviderNamespace::IODataProviderException,
								std::string("Cannot delete subscription for ").append(nodeId.toString()));
				exception->setCause(&e);
			}
		}
		MutexLock lock(*d->mutex);
		if (exception == NULL) {
			// for each callback data
			std::vector<JDataProviderPrivate::CallbackData*>::iterator callbackDataIter =
					d->callbacks.begin();
			for (; callbackDataIter != d->callbacks.end(); callbackDataIter++) {
				// if callback data for nodeId
				if (nodeId.equals(*(*callbackDataIter)->nodeId)) {
					break;
				}
			}
			if (callbackDataIter != d->callbacks.end()) {
				// delete callback and remove it from list
				delete (*callbackDataIter)->nodeId;
				delete *callbackDataIter;
				d->callbacks.erase(callbackDataIter);
			}
		}
		messageId = d->messageIdCounter++;
	}
	if (exception != NULL) {
		IODataProviderNamespace::IODataProviderException ex = ExceptionDef(
				IODataProviderNamespace::IODataProviderException,
				std::string("Deletion of subscriptions failed"));
		ex.setCause(exception);
		delete exception;
		throw ex;
	}
}

void JDataProvider::notification(JNIEnv *env, int ns, jobject id,
		jobject value) {
	ParamId* paramIdp = native2j->createParamId(env, ns, id);
	ScopeGuard<ParamId> sParamId(paramIdp);
	IODataProviderNamespace::NodeId* ioNodeId = d->converter.convertBin2io(
			*paramIdp, ns);

	std::vector<const IODataProviderNamespace::NodeData*>* ioNodeData =
			new std::vector<const IODataProviderNamespace::NodeData*>();

	UaNodeId nId = getUaNode(paramIdp);

	updateModel(nId);

	ModelType t;
	t.type = ModelType::REF;
	t.ref = getParamId(nId);

	Variant* v = native2j->getVariant(env, value, t.ref, t);
	ScopeGuard<Variant> sV(v);

	IODataProviderNamespace::Variant* ioNodeValue = d->converter.convertBin2io(
			*v, ns);

	ioNodeData->push_back(
			new IODataProviderNamespace::NodeData(*ioNodeId, ioNodeValue,
					true /* attachValues */));

	IODataProviderNamespace::Event ioEvent(time(NULL) * 1000, *ioNodeData,
			true /* attachValues */);

	for (int i = 0; i < d->callbacks.size(); i++) {
		JDataProviderPrivate::CallbackData& callbackData = *d->callbacks[i];
		if (callbackData.nodeId->equals(*ioNodeId)) {
			d->callbacks[i]->callback->valuesChanged(ioEvent);
		}
	}
}

void JDataProvider::event(JNIEnv *env, int eNs, jobject event, int pNs, jobject param, long timestamp, int severity, jstring msg, jobject value) {

	ParamId* eventIdp = native2j->createParamId(env, eNs, event);
	ScopeGuard<ParamId> sEventIdp(eventIdp);
	ParamId* paramIdp = native2j->createParamId(env, pNs, param);
	ScopeGuard<ParamId> sParamIdp(paramIdp);
	IODataProviderNamespace::NodeId* srcNodeId = d->converter.convertBin2io(
			*paramIdp, eNs);
	// fieldData
    std::vector<const IODataProviderNamespace::NodeData*>* ioFieldData
            = new std::vector<const IODataProviderNamespace::NodeData*>();
    VectorScopeGuard<const IODataProviderNamespace::NodeData> ioFieldDataSG(ioFieldData);



    UaNodeId nodeToBrowse;
	UaNodeId evt = getUaNode(eventIdp);

	UaNodeId referenceTypeId(OpcUaId_HierarchicalReferences);
	BrowseContext bc(NULL /*view*/,
	                (OpcUa_NodeId*) (const OpcUa_NodeId*) nodeToBrowse,
	                0 /*maxResultsToReturn*/,
	                OpcUa_BrowseDirection_Forward,
	                (OpcUa_NodeId*) (const OpcUa_NodeId*) referenceTypeId,
	                OpcUa_True /*includeSubtypes*/,
	                0 /*nodeClassMask*/,
	                OpcUa_BrowseResultMask_All /* resultMask */);

	UaReferenceDescriptions referenceDescriptions;
	ServiceContext sc;
	nodeBrowser->getObjectType(evt)->browse(sc, bc, referenceDescriptions);

    for (OpcUa_UInt32 i = 0; i < referenceDescriptions.length(); i++) {
    	bool changed = false;
    	changed = findFieldModel(nodeBrowser->getVariable(UaNodeId(referenceDescriptions[i].NodeId.NodeId))->dataType());
        if (native2j->mapContains(env, value, nodeBrowser->getVariable(UaNodeId(referenceDescriptions[i].NodeId.NodeId))->nodeId().toFullString().toUtf8())){
        	if (fields.count(eventIdp->toString().c_str()) == 0){
        		ModelType t;
        		t.type = ModelType::REF;
        		t.ref = getParamId(UaNodeId(referenceDescriptions[i].NodeId.NodeId));
        		fields[eventIdp->toString().c_str()][UaNodeId(referenceDescriptions[i].NodeId.NodeId).toFullString().toUtf8()] = t;
        		changed = true;
        	}
        }
		if (changed){
			native2j->updateModel(fields);
		}
    }
    //TODO End - browse to NodeBrowser?
	//Convert values

    ModelType t;
	t.type = ModelType::REF;
	t.ref = getParamId(nodeBrowser->getNode(evt)->nodeId());

	Variant* v = native2j->getVariant(env, value, "", t);
	ScopeGuard<Variant> vSG(v);

	if (v->getVariantType() != Variant::STRUCT){
		return;
	}

	const Struct& s = (Struct&)*v;
	//Create Event Params
	std::map<std::string, const Variant*> fields = s.getFields();
    for (std::map<const string, const Variant*>::const_iterator i = fields.begin();
            i != fields.end(); i++) {
    	const ParamId& pId(i->first);
		const Variant& paramValue = *i->second;
		ScopeGuard<IODataProviderNamespace::NodeId> nodeIdSG(
				d->converter.convertBin2io(pId, pNs)); // ConversionException
		IODataProviderNamespace::Variant* nodeValue =
				d->converter.convertBin2io(paramValue, pNs); // ConversionException
		ioFieldData->push_back(new IODataProviderNamespace::NodeData(
				*nodeIdSG.detach(), nodeValue, true /*attachValue*/));
    }



	// eventValue incl. sourceNodeId, message, severity, fieldData
    const char *str = env->GetStringUTFChars(msg, NULL);

	IODataProviderNamespace::OpcUaEventData* ioEventValue =
			new IODataProviderNamespace::OpcUaEventData(*srcNodeId,
			*new std::string(str), severity,
			*ioFieldDataSG.detach(), true /* attachValues*/);

	env->ReleaseStringUTFChars(msg, str);



	// eventTypeId
	IODataProviderNamespace::NodeId* ioEventTypeId =
	d->converter.convertBin2io(*eventIdp, eNs); // ConversionException
				// eventData incl. eventTypeId, eventValue
	std::vector<const IODataProviderNamespace::NodeData*>* ioEventData =
			new std::vector<const IODataProviderNamespace::NodeData*>();


	ioEventData->push_back(new IODataProviderNamespace::NodeData(
			*ioEventTypeId, ioEventValue, true /* attachValues */));
	// event incl. timeStamp, eventData

	IODataProviderNamespace::Event ioEvent(timestamp, *ioEventData,
			true /* attachValues */);
	// send IO data provider event via callbacks
	for (int i = 0; i < d->callbacks.size(); i++) {
		JDataProviderPrivate::CallbackData& callbackData = *d->callbacks[i];
		if (callbackData.nodeId->equals(*ioEventTypeId)) {
			callbackData.callback->valuesChanged(ioEvent); // SubscriberCallbackException
		}
	}








}


