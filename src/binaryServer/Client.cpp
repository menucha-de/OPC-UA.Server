#include "Client.h"
#include "ServerException.h"
#include "CachedConverterCallback.h"
#include "Event.h"
#include "EventField.h"
#include "HaSession.h"
#include "../provider/binary/common/ConversionException.h"
#include "../provider/binary/common/TimeoutException.h"
#include "../provider/binary/messages/ConverterBin2IO.h"
#include "../provider/binary/messages/dto/Array.h"
#include "../provider/binary/messages/dto/Call.h"
#include "../provider/binary/messages/dto/CallResponse.h"
#include "../provider/binary/messages/dto/Event.h"
#include "../provider/binary/messages/dto/Message.h"
#include "../provider/binary/messages/dto/MessageHeader.h"
#include "../provider/binary/messages/dto/Notification.h"
#include "../provider/binary/messages/dto/ParamId.h"
#include "../provider/binary/messages/dto/ParamMap.h"
#include "../provider/binary/messages/dto/Scalar.h"
#include "../provider/binary/messages/dto/SubscribeResponse.h"
#include "../provider/binary/messages/dto/Unsubscribe.h"
#include "../provider/binary/messages/dto/UnsubscribeResponse.h"
#include "../provider/binary/messages/dto/WriteResponse.h"
#include "../utilities/linux.h" // RegisterSignalHandler
#include <common/Exception.h>
#include <common/Mutex.h>
#include <common/MutexLock.h>
#include <common/ScopeGuard.h>
#include <common/VectorScopeGuard.h>
#include <common/logging/Logger.h>
#include <common/logging/LoggerFactory.h>
#include <ioDataProvider/NodeId.h>
#include <ioDataProvider/Scalar.h>
#include <ioDataProvider/Variant.h>
#include <sasModelProvider/base/ConverterUa2IO.h>
#include <libtrace.h> // LibT
#include <opcua_trace.h> // OPCUA_TRACE_OUTPUT_LEVEL_ALL
#include <uatrace.h> // UaTrace
#include <uaplatformlayer.h> // UaPlatformLayer
#include <uavariant.h> // UaVariant
#include <xmldocument.h> // UaXmlDocument
#include <uaclientsdk.h>
#include <sstream> // std::ostringstream
#include <time.h> // nanosleep
#include <map>
#include <vector>
#include <common/logging/ConsoleLoggerFactory.h>
#include "../../include/common/logging/JLoggerFactory.h"

using namespace CommonNamespace;
using namespace UaClientSdk;

class ClientPrivate {
	friend class Client;
private:

	class ConverterCallback: public SASModelProviderNamespace::ConverterUa2IO::ConverterCallback {
	public:

		ConverterCallback(HaSession* session) {
			this->session = session;
		}

		virtual UaStructureDefinition getStructureDefinition(
				const UaNodeId& dataTypeId) {
			return session->getStructureDefinition(dataTypeId);
		}

		virtual std::vector<UaNodeId>* getSuperTypes(const UaNodeId& typeId) {
			return session->getSuperTypes(typeId);
		}

	private:
		HaSession* session;
	};

	class SessionCallback: public HaSession::HaSessionCallback {
	public:

		SessionCallback(ClientPrivate& server) {
			this->server = &server;
		}

		virtual void dataChanged(std::vector<NodeAttributes*>& nodeAttributes)
		/* throws Exception */{
			server->dataChanged(nodeAttributes); // Exception
		}

		virtual void newEvents(
				std::vector<const BinaryServerNamespace::Event*>& events)
				/* throws Exception */{
			server->newEvents(events); // Exception
		}

		virtual void connectionStateChanged(int state){
			server->connectionStateChanged(state);
		}

	private:
		ClientPrivate* server;
	};

	// max. interval for reopening the server after exceptions
	static const int MAX_RECONNECT_DELAY = 10; // in sec

	Logger* log;
	LoggerFactory* loggerFactory;

	HaSession::Configuration sessionConf;
	MessageHandler *msgHandler;
	char* appPath;
	pthread_t binaryThread;
	SessionCallback* opcuaSessionCallback;
	HaSession* opcuaSession;
	int namespaceIndex;

	unsigned long messageIdCounter;
	SASModelProviderNamespace::CachedConverterCallback* cachedConverterUa2ioCallback;
	SASModelProviderNamespace::ConverterUa2IO* converterUa2io;

	Mutex* mutex;
	bool isListening;

	bool findFieldModel(const UaNodeId &start);
	std::map<std::string, std::map<std::string, ModelType> > fields;

	void browse(const UaNodeId& startingNode, std::string prefix,
			std::map<std::string, std::map<std::string, std::string> > &fields);

	void subscribe(Subscribe& request) /*throws ConversionException, HaSubscriptionException,
	 TimeoutException, ServerSocketException,
	 MessageDeserializerException*/;
	void unsubscribe(Unsubscribe& request) /*throws ConversionException, HaSubscriptionException,
	 TimeoutException, ServerSocketException,
	 MessageDeserializerException*/;
	ReadResponse read(Read& request) /*throws ConversionException, HaSessionException, TimeoutException,
	 ServerSocketException, MessageSerializerException*/;
	void write(Write& request) /*throws ConversionException, HaSessionException, TimeoutException,
	 ServerSocketException, MessageSerializerException*/;
	CallResponse call(Call& request) /*throws ConversionException, HaSessionException, TimeoutException,
	 ServerSocketException, MessageSerializerException*/;
	void dataChanged(
			std::vector<NodeAttributes*>& nodeAttributes) /* throws Exception */;
	void newEvents(
			std::vector<const BinaryServerNamespace::Event*>& events) /* throws Exception */;

	// Message* readMessage() /*throws TimeoutException, ServerSocketException,
	//                         MessageDeserializerException */;
	// void writeMessage(Message& message) /*throws TimeoutException, ServerSocketException,
	//                                      MessageSerializerException */;

	static void* binaryThreadRun(void* object);

	UaNodeId* convertBin2ua(
			const ParamId& paramId) /* throws ConversionException */;
	UaVariant* convertBin2ua(const Variant& value,
			const UaNodeId& destDataTypeId) /* throws ConversionException */;

	ParamId* convertUa2bin(
			const UaNodeId& nodeId) /* throws ConversionException */;
	Variant* convertUa2bin(const UaVariant& value,
			const UaNodeId& srcDataTypeId) /*throws ConversionException*/;

	void connectionStateChanged(int state);

	std::vector<std::string> checkModel(const ParamId& pId, int type);

};

Client::Client() /* throws MutexException */{
	d = new ClientPrivate();
	d->log = LoggerFactory::getLogger("Server");
	d->loggerFactory = NULL;
	d->appPath = getAppPath();
	d->opcuaSessionCallback = new ClientPrivate::SessionCallback(*d);
	d->opcuaSession = NULL;
	d->messageIdCounter = 1;
	d->cachedConverterUa2ioCallback = NULL;
	d->converterUa2io = NULL;
	d->mutex = new Mutex(); // MutexException
	d->isListening = false;
}

Client::~Client() /* throws HaSessionException, HaSubscriptionException */{
	close(); // HaSessionException, HaSubscriptionException
	delete d->mutex;
	delete d->opcuaSessionCallback;
	delete[] d->appPath;
	delete d;
}

void Client::open(Client::Options& options,
		MessageHandler *handler) /* throws ServerException, ServerSocketException, HaSessionException */{
	d->msgHandler = handler;
	registerSignalHandler();
	// initialize the XML parser
	UaXmlDocument::initParser();
	// initialize the UA Stack platform layer
	if (UaPlatformLayer::init() != 0) {
		std::ostringstream msg;
		msg << "Cannot initialize UA stack platform layer";
		throw ExceptionDef(ServerException, msg.str());
	}
	d->sessionConf.host = *options.remoteHost;
	if (options.remotePort != NULL) {
		d->sessionConf.port = *options.remotePort;
	} else {
		d->sessionConf.port = 0;
	}
	d->sessionConf.username = options.username;
	d->sessionConf.password = options.password;

	d->sessionConf.sendReceiveTimeout = 10;
	d->sessionConf.maxReconnectDelay = 10;

	if (options.publishInterval != NULL) {
		d->sessionConf.publishingInterval = *options.publishInterval;
	} else {
		d->sessionConf.publishingInterval = 100;
	}

	if (options.connectTimeout != NULL) {
		d->sessionConf.connectTimeout = *options.connectTimeout;
	} else {
		d->sessionConf.connectTimeout = 5;
	}

	if (options.sendReceiveTimeout != NULL) {
		d->sessionConf.sendReceiveTimeout = *options.sendReceiveTimeout;
	} else {
		d->sessionConf.sendReceiveTimeout = 5;
	}

	if (options.watchdogInterval != NULL) {
		d->sessionConf.watchdogInterval = *options.watchdogInterval;
	} else {
		d->sessionConf.watchdogInterval = 5;
	}

	UaTrace::TraceLevel uaAppTraceLevel = UaTrace::Errors;
	// activate app logging
//    UaTrace::TraceLevel uaAppTraceLevel = UaTrace::NoTrace;
//    if (0 == loggingConf.uaAppLogLevel.compare("error")) {
//    	uaAppTraceLevel = UaTrace::Errors;
//    } else if (0 == loggingConf.uaAppLogLevel.compare("warn")) {
//        uaAppTraceLevel = UaTrace::Warning;
//    } else if (0 == loggingConf.uaAppLogLevel.compare("info")) {
//        uaAppTraceLevel = UaTrace::Info;
//    } else if (0 == loggingConf.uaAppLogLevel.compare("debug")) {
//        uaAppTraceLevel = UaTrace::CtorDtor; // incl. InterfaceCall
//    } else if (0 == loggingConf.uaAppLogLevel.compare("trace")) {
//        uaAppTraceLevel = UaTrace::Data; // incl. ProgramFlow
//    }
	LibT::initTrace(uaAppTraceLevel, 1000 /*maxTraceEntries*/, 0,
			UaString("opcua.log"), "uaclient");
	LibT::setTraceActive(uaAppTraceLevel != UaTrace::NoTrace);
	// activate stack logging
	OpcUa_UInt32 uaStackTraceLevel = OPCUA_TRACE_OUTPUT_LEVEL_NONE;
	//    if (0 == loggingConf.uaStackLogLevel.compare("error")) {
	//        uaStackTraceLevel = OPCUA_TRACE_OUTPUT_LEVEL_ERROR;
	//    } else if (0 == loggingConf.uaStackLogLevel.compare("warn")) {
	//        uaStackTraceLevel = OPCUA_TRACE_OUTPUT_LEVEL_WARNING;
	//    } else if (0 == loggingConf.uaStackLogLevel.compare("info")) {
	//        uaStackTraceLevel = OPCUA_TRACE_OUTPUT_LEVEL_INFO; // incl. System
	//    } else if (0 == loggingConf.uaStackLogLevel.compare("debug")) {
	//        uaStackTraceLevel = OPCUA_TRACE_OUTPUT_LEVEL_DEBUG;
	//    } else if (0 == loggingConf.uaStackLogLevel.compare("trace")) {
	//        uaStackTraceLevel = OPCUA_TRACE_OUTPUT_LEVEL_CONTENT;
	//    }
	UaPlatformLayer::changeTraceSettings(
			uaStackTraceLevel != OPCUA_TRACE_OUTPUT_LEVEL_NONE
			/*traceEnabled*/, uaStackTraceLevel);
	LibT::setStackTraceActive(
			uaStackTraceLevel != OPCUA_TRACE_OUTPUT_LEVEL_NONE);
	d->log = LoggerFactory::getLogger("Server");
//	d->loggerFactory = new LoggerFactory(*new ConsoleLoggerFactory(),
//			true /*attachValues*/);
//	d->log = LoggerFactory::getLogger("main");
	// open a server socket
	//    ServerSocket* serverSocket = new ServerSocket(d->serverConf.acceptTimeout,
	//            d->serverConf.sendReceiveTimeout, true /*reuseAddress*/);
	//    ScopeGuard<ServerSocket> serverSocketSG(serverSocket);
	//    serverSocket->open(d->serverConf.port); // ServerSocketException
	// create a session to OPC UA server
	HaSession* opcuaSession = new HaSession(d->sessionConf,
			*d->opcuaSessionCallback); // MutexException
	ScopeGuard<HaSession> opcuaSessionSG(opcuaSession);
	opcuaSession->open(); // HaSessionException
	// the binary interface does not support namespaces => use the last loaded namespace
	d->namespaceIndex = opcuaSession->getNamespaceTable().length() - 1;
	// create converter incl. cache (OpcUa <-> IODataProvider)
	d->cachedConverterUa2ioCallback =
			new SASModelProviderNamespace::CachedConverterCallback(
					*new ClientPrivate::ConverterCallback(opcuaSession),
					true /*attachValues*/); //MutexException
	d->converterUa2io = new SASModelProviderNamespace::ConverterUa2IO(
			*d->cachedConverterUa2ioCallback);

	//d->serverSocket = serverSocketSG.detach();
	d->opcuaSession = opcuaSessionSG.detach();
	// start the thread for processing the incoming messages via the binary interface
	d->isListening = true;

	//pthread_create(&d->binaryThread, NULL, &ServerPrivate::binaryThreadRun, d);
}

ReadResponse Client::read(Read& request) {
	return d->read(request);
}

void Client::subscribe(Subscribe& request) {
	return d->subscribe(request);
}

std::string Client::getSessionId() {
	return d->opcuaSession->getSessionId();
}

std::map<std::string, std::map<std::string, std::string> > Client::browse(
		int ns, int startNode, const std::string prefix) {
	// Root as starting node for recursive browsing
	UaNodeId startingNode(startNode, ns);
	// Start recursive browsing
	std::map<std::string, std::map<std::string, std::string> > fields;
	d->browse(startingNode, prefix, fields);
	return fields;
}

std::map<std::string, std::map<std::string, std::string> > Client::browse(
		int ns, const std::string startNode, const std::string prefix) {
	// Root as starting node for recursive browsing
	UaNodeId startingNode(startNode.c_str(), ns);
	// Start recursive browsing
	std::map<std::string, std::map<std::string, std::string> > fields;
	d->browse(startingNode, prefix, fields);
	return fields;
}

void Client::unsubscribe(Unsubscribe& request) {
	return d->unsubscribe(request);
}

void Client::write(Write& request) {
	return d->write(request);
}

CallResponse Client::call(Call& request) {
	return d->call(request);
}

std::vector<std::string> Client::checkModel(const ParamId &pId, int type){
	return d->checkModel(pId, type);
}

void Client::close() /* throws HaSessionException, HaSubscriptionException */{
	// set flag for stopping the threads
	bool isListening;
	{
		MutexLock lock(*d->mutex);
		isListening = d->isListening;
		d->isListening = false;
	}
	// close OPC UA session (avoids receiving of further events)
	if (d->opcuaSession != NULL) {
		d->opcuaSession->close(); // HaSessionException, HaSubscriptionException
	}

	if (isListening) {
		// wait for end of thread
		pthread_join(d->binaryThread, NULL /*return*/);
	}
	delete d->opcuaSession;
	d->opcuaSession = NULL;
	delete d->converterUa2io;
	d->converterUa2io = NULL;
	delete d->cachedConverterUa2ioCallback;
	d->cachedConverterUa2ioCallback = NULL;
	//	delete d->loggerFactory;
	//	d->loggerFactory = NULL;
	// clean up the UA Stack platform layer
	UaPlatformLayer::cleanup();
	// clean up the XML parser
	UaXmlDocument::cleanupParser();
}

void ClientPrivate::subscribe(Subscribe& request)
/*throws ConversionException, HaSubscriptionException, TimeoutException, 
 * ServerSocketException, MessageDeserializerException*/{
	try {
		std::vector<const UaNodeId*>* nodeIds =
				new std::vector<const UaNodeId*>();
		VectorScopeGuard<const UaNodeId> nodeIdsSG(nodeIds);
		// convert ParamId to UaNodeId
		UaNodeId* nodeId = convertBin2ua(request.getParamId()); // ConversionException
		nodeIds->push_back(nodeId);
		// subscribe
		std::vector<NodeAttributes*> nodeAttributes;
		std::map<UaNodeId, std::vector<BinaryServerNamespace::EventField*>*> eventFields;
		opcuaSession->subscribe(*nodeIds, nodeAttributes, eventFields); // HaSubscriptionException
		// for each node attribute
		for (int i = 0; i < nodeAttributes.size(); i++) {
			// preload attribute type info
			cachedConverterUa2ioCallback->preload(
					*nodeAttributes[i]->getDataType());
			delete nodeAttributes[i];
		}
		// for each event type
		for (std::map<UaNodeId, std::vector<BinaryServerNamespace::EventField*>*>::const_iterator it =
				eventFields.begin(); it != eventFields.end(); it++) {
			// preload event type infos
			cachedConverterUa2ioCallback->preload(it->first);
			std::vector<BinaryServerNamespace::EventField*>* eventFields =
					it->second;
			// for each event field of event type
			for (int i = 0; i < eventFields->size(); i++) {
				BinaryServerNamespace::EventField* eventField =
						(*eventFields)[i];
				// preload event field infos
				cachedConverterUa2ioCallback->preload(
						*eventField->getDataTypeId());
				delete eventField;
			}
			delete eventFields;
		}
	} catch (Exception& e) {
		// send response
		SubscribeResponse response(request.getMessageHeader().getMessageId(),
				Status::INVALID_PARAMETER);
		//writeMessage(response); /* TimeoutException, ServerSocketException, MessageSerializerException */
		throw;
	}
	// send response
	SubscribeResponse response(request.getMessageHeader().getMessageId(),
			Status::SUCCESS);
	//writeMessage(response); /* TimeoutException, ServerSocketException, MessageSerializerException */
}

void ClientPrivate::unsubscribe(Unsubscribe& request)
/*throws ConversionException, HaSubscriptionException, TimeoutException, 
 * ServerSocketException, MessageDeserializerException*/{
	try {
		std::vector<const UaNodeId*>* nodeIds =
				new std::vector<const UaNodeId*>();
		VectorScopeGuard<const UaNodeId> nodeIdsSG(nodeIds);
		// convert ParamId to UaNodeId
		UaNodeId* nodeId = convertBin2ua(request.getParamId()); // ConversionException
		nodeIds->push_back(nodeId);
		// subscribe
		opcuaSession->unsubscribe(*nodeIds); // HaSubscriptionException
	} catch (Exception& e) {
		// send response
		UnsubscribeResponse response(request.getMessageHeader().getMessageId(),
				Status::INVALID_PARAMETER);
		//writeMessage(response); /* TimeoutException, ServerSocketException, MessageSerializerException */
		throw;
	}
	// send response
	UnsubscribeResponse response(request.getMessageHeader().getMessageId(),
			Status::SUCCESS);
	//writeMessage(response); /* TimeoutException, ServerSocketException, MessageSerializerException */
}

ReadResponse ClientPrivate::read(Read& request)
/*throws ConversionException, HaSessionException, TimeoutException, 
 * ServerSocketException, MessageDeserializerException*/{
	Variant* paramValue;
	try {
		std::vector<const UaNodeId*>* nodeIds =
				new std::vector<const UaNodeId*>();
		VectorScopeGuard<const UaNodeId> nodeIdsSG(nodeIds);
		// convert ParamId to UaNodeId
		UaNodeId* nodeId = convertBin2ua(request.getParamId()); // ConversionException
		nodeIds->push_back(nodeId);
		// read value incl. data type
		std::vector<NodeAttributes*>* values =
				new std::vector<NodeAttributes*>();
		opcuaSession->read(*nodeIds, *values); // HaSessionException
		VectorScopeGuard<NodeAttributes> valuesSG(values);
		NodeAttributes& nodeAttr = *(*values)[0];
		if (nodeAttr.getException() != NULL) {
			throw *nodeAttr.getException();
		}

		// convert UaVariant to Variant
		paramValue = convertUa2bin(*nodeAttr.getValue(),
				*nodeAttr.getDataType()); // ConversionException
	} catch (Exception& e) {
		// send response

		ReadResponse response(request.getMessageHeader().getMessageId(),
				Status::INVALID_PARAMETER);
		//writeMessage(response); /* TimeoutException, ServerSocketException, MessageSerializerException */
		throw;
	}
	// send response
	ReadResponse response(request.getMessageHeader().getMessageId(),
			Status::SUCCESS, true /* attachValues */);
	response.setParamId(new ParamId(request.getParamId()));
	response.setParamValue(paramValue);
	//writeMessage(response); /* TimeoutException, ServerSocketException, MessageSerializerException */
	return response;
}

void ClientPrivate::write(Write& request)
/*throws ConversionException, HaSessionException, TimeoutException, 
 * ServerSocketException, MessageDeserializerException*/{
	try {
		std::vector<const UaNodeId*>* nodeIds =
				new std::vector<const UaNodeId*>();
		VectorScopeGuard<const UaNodeId> nodeIdsSG(nodeIds);
		// convert ParamId to UaNodeId
		UaNodeId* nodeId = convertBin2ua(request.getParamId()); // ConversionException
		nodeIds->push_back(nodeId);
		// read data type
		std::vector<NodeAttributes*>* readValues = new std::vector<
				NodeAttributes*>();
		opcuaSession->read(*nodeIds, *readValues); // HaSessionException
		VectorScopeGuard<NodeAttributes> readValuesSG(readValues);
		NodeAttributes& nodeAttr = *(*readValues)[0];
		if (nodeAttr.getException() != NULL) {
			throw *nodeAttr.getException();
		}

		// convert Variant to UaVariant
		UaVariant* value = convertBin2ua(request.getParamValue(),
				*nodeAttr.getDataType()); // ConversionException
		ScopeGuard<UaVariant> valueSG(value);
		std::map<const UaNodeId*, const UaVariant*> nodes;
		nodes[nodeId] = value;
		// write values
		opcuaSession->write(nodes); // HaSessionException
	} catch (Exception& e) {
		// send response

		WriteResponse response(request.getMessageHeader().getMessageId(),
				Status::INVALID_PARAMETER);
		//writeMessage(response); /* TimeoutException, ServerSocketException, MessageSerializerException */
		throw;
	}
	// send response
	WriteResponse response(request.getMessageHeader().getMessageId(),
			Status::SUCCESS);
	//writeMessage(response); /* TimeoutException, ServerSocketException, MessageSerializerException */
}

std::vector<std::string> ClientPrivate::checkModel(const ParamId &pId, int type){
	// convert ParamId to UaNodeId
	UaNodeId* nodeId = convertBin2ua(pId); // ConversionException
	ScopeGuard<UaNodeId> methodIdSG(nodeId);
	std::vector<std::string> names;
	//CALL
	if (type == 0){
		UaArguments inputArgs;
		UaArguments outputArgs;
		opcuaSession->getMethodArguments(*nodeId, inputArgs, outputArgs);
		for (int i = 0; i < inputArgs.length(); i++) {
			UaArgument *a = new UaArgument(inputArgs[i]);
			ScopeGuard<UaArgument> sgA(a);
			ModelType t;
			t.type = ModelType::REF;
			t.ref = a->getDataType().toFullString().toUtf8();
			names.push_back(a->getName().toUtf8());
			fields[pId.toString()][a->getName().toUtf8()] = t;
			findFieldModel(a->getDataType());
			msgHandler->modelUpdated(fields);
		}
		return names;
	//READ/WRITE
	} else {
		ModelType t;
		t.type = ModelType::REF;
		t.ref = opcuaSession->getVariable(*nodeId).dataType().toFullString().toUtf8();
		fields[pId.toString()][t.ref] = t;
		findFieldModel(opcuaSession->getVariable(*nodeId).dataType());
		names.push_back(t.ref);
		msgHandler->modelUpdated(fields);
		return names;
	}
}



CallResponse ClientPrivate::call(Call& request)
/*throws ConversionException, HaSessionException, TimeoutException, 
 * ServerSocketException, MessageDeserializerException*/{
	std::vector<const Variant*>* outputParams;
	try {
		// convert ParamId to UaNodeId
		UaNodeId* methodId = convertBin2ua(request.getMethodId()); // ConversionException
		ScopeGuard<UaNodeId> methodIdSG(methodId);
		UaNodeId* objectId = convertBin2ua(request.getParamId()); // ConversionException
		ScopeGuard<UaNodeId> objectIdSG(objectId);
		// get method arguments
		UaArguments inputArgs;
		UaArguments outputArgs;
		opcuaSession->getMethodArguments(*methodId, inputArgs, outputArgs);
		const std::vector<const Variant*>& inputParams =
				request.getParamList().getElements();
		if (inputParams.size() != inputArgs.length()) {
			std::ostringstream msg;
			msg << "Invalid count of input arguments: server="
					<< inputArgs.length() << ",caller=" << inputParams.size();
			throw ExceptionDef(ServerException, msg.str());
		}

		std::vector<UaVariant*>* inputValues = new std::vector<UaVariant*>();
		VectorScopeGuard<UaVariant> inputValuesSG(inputValues);
		// for each input parameter
		for (int i = 0; i < inputParams.size(); i++) {
			const Variant& value = *inputParams[i];
			UaNodeId destDataType(inputArgs[i].DataType);
			// convert Variant to UaVariant
			UaVariant* v = convertBin2ua(value, destDataType); // ConversionException
			inputValues->push_back(v);
		}
		// call method
		std::vector<UaVariant*>* outputValues = opcuaSession->call(*methodId,
				*objectId, *inputValues); // HaSessionException
		VectorScopeGuard<UaVariant> outputValuesSG(outputValues);
		outputParams = new std::vector<const Variant*>();
		// for each output parameter
		for (int i = 0; i < outputValues->size(); i++) {
			UaVariant& value = *(*outputValues)[i];
			// convert UaVariant to Variant
			Variant* v = convertUa2bin(value, outputArgs[i].DataType); // ConversionException
			outputParams->push_back(v);
		}
	} catch (Exception& e) {
		// send response
		const unsigned long* errorCode = e.getErrorCode();
		if (errorCode != NULL) {
			CallResponse response(request.getMessageHeader().getMessageId(),
					Status::APPLICATION_ERROR);
			response.setMethodId(&request.getMethodId());
			response.setParamId(&request.getParamId());
			std::vector<const Variant*> elements;
			Scalar e1;
			e1.setInt(*errorCode);
			elements.push_back(&e1);
			std::vector<const Variant*> errorMsgElems;
			const Array e2(Scalar::CHAR, errorMsgElems);
			elements.push_back(&e2);
			ParamList paramList(elements);
			response.setParamList(&paramList);
			//writeMessage(response); /* TimeoutException, ServerSocketException, MessageSerializerException */
		} else {
			log->info("Error while calling %s", e.getMessage().c_str());
			//TODO throw Exception
			CallResponse response(request.getMessageHeader().getMessageId(),
					Status::INVALID_PARAMETER);
			return response;
			//writeMessage(response); /* TimeoutException, ServerSocketException, MessageSerializerException */
		}
		throw;
	}
	// send response
	CallResponse response(request.getMessageHeader().getMessageId(),
			Status::SUCCESS, true /* attachValues */);
	response.setMethodId(new ParamId(request.getMethodId()));
	response.setParamId(new ParamId(request.getParamId()));
	response.setParamList(new ParamList(*outputParams, true /*attachValues*/));
	//writeMessage(response); /* TimeoutException, ServerSocketException, MessageSerializerException */
	return response;
}

void ClientPrivate::dataChanged(std::vector<NodeAttributes*>& nodeAttributes)
/* throws Exception */{
	ServerException* exception = NULL;
	std::map<const ParamId*, const Variant*>* elements = new std::map<
			const ParamId*, const Variant*>();
	// for each node
	for (int i = 0; i < nodeAttributes.size(); i++) {
		NodeAttributes& nodeAttr = *nodeAttributes[i];
		if (nodeAttr.getException() != NULL) {
			if (exception == NULL) {
				std::ostringstream msg;
				msg << "Cannot send notification for subscription of "
						<< nodeAttr.getNodeId().toXmlString().toUtf8();
				exception = new ExceptionDef(ServerException, msg.str());
				exception->setCause(nodeAttr.getException());
			}
			continue;
		}
		try {
			ParamId* paramId = convertUa2bin(nodeAttr.getNodeId()); // ConversionException
			ScopeGuard<ParamId> paramIdSG(paramId);
			Variant* value = convertUa2bin(*nodeAttr.getValue(),
					*nodeAttr.getDataType()); // ConversionException
			(*elements)[paramIdSG.detach()] = value;
		} catch (Exception& e) {
			if (exception == NULL) {
				std::ostringstream msg;
				msg << "Cannot send notification for subscription of "
						<< nodeAttr.getNodeId().toXmlString().toUtf8();
				exception = new ExceptionDef(ServerException, msg.str());
				exception->setCause(&e);
			}
		}
	}
	Notification notification(messageIdCounter++,
			*new ParamMap(*elements, true /*attachValues*/),
			true /*attachValues*/);
	if (elements->size() > 0) {
		try {
			msgHandler->notificationReceived(notification);
			//writeMessage(notification); // TimeoutException, ServerSocketException, MessageSerializerException
		} catch (Exception& e) {
			if (exception == NULL) {
				std::ostringstream msg;
				msg << "Cannot send data change notification";
				exception = new ExceptionDef(ServerException, msg.str());
				exception->setCause(&e);
			}
		}
	}
	if (exception != NULL) {

		ScopeGuard<Exception> exceptionSG(exception);
		throw *exception;
	}
}

void ClientPrivate::connectionStateChanged(int state){
	msgHandler->connectionStateChanged(state);
}

void ClientPrivate::newEvents(
		std::vector<const BinaryServerNamespace::Event*>& events)
		/* throws Exception */{
	ServerException* exception = NULL;
	// for each event
	for (int i = 0; i < events.size(); i++) {
		const BinaryServerNamespace::Event& event = *events[i];
		ParamId* eventTypeId = NULL;
		ParamId* paramId = NULL;
		long long timeStamp;
		OpcUa_UInt16 severity;
		const std::string* message = NULL;
		std::map<const ParamId*, const Variant*>* elements = new std::map<
				const ParamId*, const Variant*>();
		try {
			eventTypeId = convertUa2bin(event.getEventTypeId()); // ConversionException
			std::vector<BinaryServerNamespace::EventField*>* eventFields =
					event.getEventFields();
			// for each event field
			for (int j = 0; j < eventFields->size(); j++) {
				BinaryServerNamespace::EventField& eventField =
						*(*eventFields)[j];
				if (eventField.getException() != NULL) {
					throw *eventField.getException();
				}
				if (0 == eventField.getNodeId().namespaceIndex()) {
					switch (eventField.getNodeId().identifierNumeric()) {
					case OpcUaId_BaseEventType_SourceNode: {
						UaNodeId nodeId;
						eventField.getValue()->toNodeId(nodeId);
						paramId = convertUa2bin(nodeId); // ConversionException
						break;
					}
					case OpcUaId_BaseEventType_Time:
					case OpcUaId_BaseEventType_Severity:
					case OpcUaId_BaseEventType_Message: {
						IODataProviderNamespace::Scalar* v =
								static_cast<IODataProviderNamespace::Scalar*>(converterUa2io->convertUa2io(
										*eventField.getValue(),
										*eventField.getDataTypeId())); // ConversionException
						switch (eventField.getNodeId().identifierNumeric()) {
						case OpcUaId_BaseEventType_Time:
							// received time stamp in 100 ns
							// 01.01.1601 - 01.01.1970: 11644473600 seconds
							timeStamp = v->getLLong() / 10 / 1000
									- 11644473600000;
							break;
						case OpcUaId_BaseEventType_Severity:
							severity = v->getUInt();
							break;
						case OpcUaId_BaseEventType_Message:
							message = new std::string(*v->getString());
							break;
						}
						delete v;
						break;
					}
					}
				} else {
					// add value to event elements
					ParamId* paramId = convertUa2bin(eventField.getNodeId()); // ConversionException
					ScopeGuard<ParamId> paramIdSG(paramId);
					Variant* value = convertUa2bin(*eventField.getValue(),
							*eventField.getDataTypeId()); // ConversionException
					(*elements)[paramIdSG.detach()] = value;
				}
			}
		} catch (Exception& e) {
			delete eventTypeId;
			delete paramId;
			delete message;
			ParamMap paramMap(*elements, true /*attachValues*/);
			if (exception == NULL) {
				std::ostringstream msg;
				msg << "Cannot prepare event for sending";
				exception = new ExceptionDef(ServerException, msg.str());
				exception->setCause(&e);
			}
			// continue with next event
			continue;
		}
		try {
			Event ev(messageIdCounter++, *eventTypeId, *paramId, timeStamp,
					severity, *message,
					*new ParamMap(*elements, true /*attachValues*/),
					true /*attachValues*/);
			msgHandler->eventReceived(ev);
			//writeMessage(ev); // TimeoutException, ServerSocketException, MessageSerializerException
		} catch (Exception& e) {
			if (exception == NULL) {
				std::ostringstream msg;
				msg << "Cannot send event";
				exception = new ExceptionDef(ServerException, msg.str());
				exception->setCause(&e);
			}
		}
	}
	if (exception != NULL) {

		ScopeGuard<Exception> exceptionSG(exception);
		throw *exception;
	}
}

void ClientPrivate::browse(const UaNodeId& startingNode, std::string prefix,
		std::map<std::string, std::map<std::string, std::string> > &fields) {
	std::map<std::string, std::string> values;
	UaStatus status;
	UaByteString continuationPoint;
	UaReferenceDescriptions referenceDescriptions;
	ServiceSettings serviceSettings;
	BrowseContext browseContext;

	/*********************************************************************
	 Browse Server
	 **********************************************************************/
	status = opcuaSession->browse(serviceSettings, startingNode, browseContext,
			continuationPoint, referenceDescriptions);
	/*********************************************************************/
	if (status.isBad()) {
		log->debug(
				"** Error: UaSession::browse of NodeId = %s failed [ret=%s]\n",
				startingNode.toFullString().toUtf8(),
				status.toString().toUtf8());
	} else {
		//log->info("Status o.k. %d %d", referenceDescriptions.length(), continuationPoint.length());
		OpcUa_UInt32 i;
		for (i = 0; i < referenceDescriptions.length(); i++) {
			values.clear();
			if (referenceDescriptions[i].NodeClass & OpcUa_NodeClass_Object)
				values["NodeClass"] = "Object";
			if (referenceDescriptions[i].NodeClass & OpcUa_NodeClass_Variable)
				values["NodeClass"] = "Variable";
			if (referenceDescriptions[i].NodeClass & OpcUa_NodeClass_Method)
				values["NodeClass"] = "Method";
			if (referenceDescriptions[i].NodeClass & OpcUa_NodeClass_ObjectType)
				values["NodeClass"] = "ObjectType";
			if (referenceDescriptions[i].NodeClass
					& OpcUa_NodeClass_VariableType)
				values["NodeClass"] = "VariableType";
			if (referenceDescriptions[i].NodeClass
					& OpcUa_NodeClass_ReferenceType)
				values["NodeClass"] = "ReferenceType";
			if (referenceDescriptions[i].NodeClass & OpcUa_NodeClass_DataType)
				values["NodeClass"] = "DataType";
			if (referenceDescriptions[i].NodeClass & OpcUa_NodeClass_View)
				values["NodeClass"] = "View";

			std::stringstream number;
			bool isKnownIdentifier = true;
			UaNodeId nodeId(referenceDescriptions[i].NodeId.NodeId);
			values["IdentifierFull"] = std::string(nodeId.toFullString().toUtf8());
			if (nodeId.identifierType() == OpcUa_IdentifierType_Numeric) {
				number << nodeId.identifierNumeric();
				values["Identifier"] = number.str();
				number.str("");
				number.clear();
				values["IdentifierType"] = "Numeric";
			} else if (nodeId.identifierType() == OpcUa_IdentifierType_String) {
				values["Identifier"] = std::string(
						UaString(nodeId.identifierString()).toUtf8());
				values["IdentifierType"] = "String";
			} else {
				isKnownIdentifier = false;
			}

			if (isKnownIdentifier) {
				UaQualifiedName browseName(referenceDescriptions[i].BrowseName);
				values["BrowseName"] = browseName.toString().toUtf8();
				number << prefix << "." << values["BrowseName"];
				values["TreeName"] = number.str();
				number.str("");
				number.clear();
				number << nodeId.namespaceIndex();
				values["NamespaceIndex"] = number.str();
				number.str("");
				number.clear();
				fields[nodeId.toFullString().toUtf8()] = values;
			}
		}

		// Check if the continuation point was set -> call browseNext
		while (continuationPoint.length() > 0) {
			/*********************************************************************
			 Browse remaining nodes in the Server
			 **********************************************************************/
			status = opcuaSession->browseNext(serviceSettings, OpcUa_False,
					continuationPoint, referenceDescriptions);
			/*********************************************************************/
			if (status.isBad()) {
				log->debug(
						"** Error: UaSession::browse of NodeId = %s failed [ret=%s] **\n",
						startingNode.toFullString().toUtf8(),
						status.toString().toUtf8());
			} else {
				for (i = 0; i < referenceDescriptions.length(); i++) {
					if (referenceDescriptions[i].NodeClass
							& OpcUa_NodeClass_Object)
						values["NodeClass"] = "Object";
					if (referenceDescriptions[i].NodeClass
							& OpcUa_NodeClass_Variable)
						values["NodeClass"] = "Variable";
					if (referenceDescriptions[i].NodeClass
							& OpcUa_NodeClass_Method)
						values["NodeClass"] = "Method";
					if (referenceDescriptions[i].NodeClass
							& OpcUa_NodeClass_ObjectType)
						values["NodeClass"] = "ObjectType";
					if (referenceDescriptions[i].NodeClass
							& OpcUa_NodeClass_VariableType)
						values["NodeClass"] = "VariableType";
					if (referenceDescriptions[i].NodeClass
							& OpcUa_NodeClass_ReferenceType)
						values["NodeClass"] = "ReferenceType";
					if (referenceDescriptions[i].NodeClass
							& OpcUa_NodeClass_DataType)
						values["NodeClass"] = "DataType";
					if (referenceDescriptions[i].NodeClass
							& OpcUa_NodeClass_View)
						values["NodeClass"] = "View";

					std::stringstream number;
					bool isKnownIdentifier = true;
					UaNodeId nodeId(referenceDescriptions[i].NodeId.NodeId);
					values["IdentifierFull"] = std::string(nodeId.toFullString().toUtf8());
					if (nodeId.identifierType()
							== OpcUa_IdentifierType_Numeric) {
						number << nodeId.identifierNumeric();
						values["Identifier"] = number.str();
						number.str("");
						number.clear();
						values["IdentifierType"] = "Numeric";
					} else if (nodeId.identifierType()
							== OpcUa_IdentifierType_String) {
						values["Identifier"] = std::string(
								UaString(nodeId.identifierString()).toUtf8());
						values["IdentifierType"] = "String";
					} else {
						isKnownIdentifier = false;
					}

					if (isKnownIdentifier) {
						UaQualifiedName browseName(
								referenceDescriptions[i].BrowseName);
						values["BrowseName"] = browseName.toString().toUtf8();
						number << prefix << "." << values["BrowseName"];
						values["TreeName"] = number.str();
						number.str("");
						number.clear();
						number << nodeId.namespaceIndex();
						values["NamespaceIndex"] = number.str();
						number.str("");
						number.clear();
						fields[nodeId.toFullString().toUtf8()] = values;
					}
				}
			}
		}
	}
}


bool ClientPrivate::findFieldModel(const UaNodeId &start) {
	bool changed = false;
	if (fields.count(start.toFullString().toUtf8())){
		return changed;
	}
	UaStructureDefinition def = opcuaSession->getStructureDefinition(start);
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
		std::vector<UaNodeId>* superTypes = opcuaSession->getSuperTypes(start); // ConversionException
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



UaNodeId* ClientPrivate::convertBin2ua(
		const ParamId& paramId) /* throws ConversionException */{
	ConverterBin2IO converterIO2bin;
	IODataProviderNamespace::NodeId* nid = converterIO2bin.convertBin2io(
			paramId, namespaceIndex); // ConversionException
	ScopeGuard<IODataProviderNamespace::NodeId> nidSG(nid);

	return converterUa2io->convertIo2ua(*nid); // ConversionException
}

ParamId* ClientPrivate::convertUa2bin(
		const UaNodeId& nodeId) /* throws ConversionException */{
	IODataProviderNamespace::NodeId* nid = converterUa2io->convertUa2io(nodeId); // ConversionException
	ScopeGuard<IODataProviderNamespace::NodeId> nidSG(nid);
	ConverterBin2IO converterIo2bin;

	return converterIo2bin.convertIo2bin(*nid); // ConversionException
}

UaVariant* ClientPrivate::convertBin2ua(const Variant& value,
		const UaNodeId& destDataTypeId)
		/* throws ConversionException */{
	ConverterBin2IO converterIo2bin;
	IODataProviderNamespace::Variant* v = converterIo2bin.convertBin2io(value,
			namespaceIndex); // ConversionException
	ScopeGuard<IODataProviderNamespace::Variant> vSG(v);

	return converterUa2io->convertIo2ua(*v, destDataTypeId); // ConversionException
}

Variant* ClientPrivate::convertUa2bin(const UaVariant& value,
		const UaNodeId& srcDataTypeId)
		/*throws ConversionException*/{
	IODataProviderNamespace::Variant* v = converterUa2io->convertUa2io(value,
			srcDataTypeId); // ConversionException
	ScopeGuard<IODataProviderNamespace::Variant> vSG(v);
	ConverterBin2IO converterIo2bin;
	return converterIo2bin.convertIo2bin(*v); // ConversionException
}
