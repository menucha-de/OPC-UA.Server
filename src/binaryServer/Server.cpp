#include "Server.h"
#include "ServerException.h"
#include "CachedConverterCallback.h"
#include "ConfigLoader.h"
#include "Event.h"
#include "EventField.h"
#include "HaSession.h"
#include "logging/ClientSdkLoggerFactory.h"
#include "../provider/binary/common/ConversionException.h"
#include "../provider/binary/common/ServerSocket.h"
#include "../provider/binary/common/ServerSocketException.h"
#include "../provider/binary/common/TimeoutException.h"
#include "../provider/binary/messages/BinaryMessageDeserializer.h"
#include "../provider/binary/messages/BinaryMessageSerializer.h"
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
#include "../provider/binary/messages/dto/Read.h"
#include "../provider/binary/messages/dto/ReadResponse.h"
#include "../provider/binary/messages/dto/Scalar.h"
#include "../provider/binary/messages/dto/Subscribe.h"
#include "../provider/binary/messages/dto/SubscribeResponse.h"
#include "../provider/binary/messages/dto/Unsubscribe.h"
#include "../provider/binary/messages/dto/UnsubscribeResponse.h"
#include "../provider/binary/messages/dto/Write.h"
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
#include <sstream> // std::ostringstream
#include <time.h> // nanosleep
#include <map>
#include <vector>

using namespace CommonNamespace;

class ServerPrivate {
    friend class Server;
private:

    class ConverterCallback : public SASModelProviderNamespace::ConverterUa2IO::ConverterCallback {
    public:

        ConverterCallback(HaSession* session) {
            this->session = session;
        }

        virtual UaStructureDefinition getStructureDefinition(const UaNodeId& dataTypeId) {
            return session->getStructureDefinition(dataTypeId);
        }

        virtual std::vector<UaNodeId>* getSuperTypes(const UaNodeId& typeId) {
            return session->getSuperTypes(typeId);
        }

    private:
        HaSession* session;
    };

    class SessionCallback : public HaSession::HaSessionCallback {
    public:

        SessionCallback(ServerPrivate& server) {
            this->server = &server;
        }

        virtual void dataChanged(std::vector<NodeAttributes*>& nodeAttributes)
        /* throws Exception */ {
            server->dataChanged(nodeAttributes); // Exception
        }

        virtual void newEvents(std::vector<const BinaryServerNamespace::Event*>& events)
        /* throws Exception */ {
            server->newEvents(events); // Exception
        }
    private:
        ServerPrivate* server;
    };

    // max. interval for reopening the server after exceptions
    static const int MAX_RECONNECT_DELAY = 10; // in sec

    Logger* log;
    LoggerFactory* loggerFactory;

    ConfigLoader::ServerConfiguration serverConf;
    HaSession::Configuration sessionConf;

    char* appPath;
    ServerSocket* serverSocket;
    pthread_t binaryThread;
    SessionCallback* opcuaSessionCallback;
    HaSession* opcuaSession;
    int namespaceIndex;
    unsigned long messageIdCounter;
    SASModelProviderNamespace::CachedConverterCallback* cachedConverterUa2ioCallback;
    SASModelProviderNamespace::ConverterUa2IO* converterUa2io;

    Mutex* mutex;
    bool isListening;

    void subscribe(Subscribe& request) /*throws ConversionException, HaSubscriptionException, 
                                        TimeoutException, ServerSocketException, 
                                        MessageDeserializerException*/;
    void unsubscribe(Unsubscribe& request) /*throws ConversionException, HaSubscriptionException, 
                                            TimeoutException, ServerSocketException, 
                                            MessageDeserializerException*/;
    void read(Read& request) /*throws ConversionException, HaSessionException, TimeoutException, 
                              ServerSocketException, MessageSerializerException*/;
    void write(Write& request) /*throws ConversionException, HaSessionException, TimeoutException, 
                                ServerSocketException, MessageSerializerException*/;
    void call(Call& request) /*throws ConversionException, HaSessionException, TimeoutException, 
                              ServerSocketException, MessageSerializerException*/;
    void dataChanged(std::vector<NodeAttributes*>& nodeAttributes) /* throws Exception */;
    void newEvents(std::vector<const BinaryServerNamespace::Event*>& events) /* throws Exception */;

    Message* readMessage() /*throws TimeoutException, ServerSocketException, 
                            MessageDeserializerException */;
    void writeMessage(Message& message) /*throws TimeoutException, ServerSocketException,
                                         MessageSerializerException */;

    static void* binaryThreadRun(void* object);

    UaNodeId* convertBin2ua(const ParamId& paramId) /* throws ConversionException */;
    UaVariant* convertBin2ua(const Variant& value,
            const UaNodeId& destDataTypeId) /* throws ConversionException */;

    ParamId* convertUa2bin(const UaNodeId& nodeId) /* throws ConversionException */;
    Variant* convertUa2bin(const UaVariant& value,
            const UaNodeId& srcDataTypeId) /*throws ConversionException*/;
};

Server::Server() /* throws MutexException */ {
    d = new ServerPrivate();
    d->log = LoggerFactory::getLogger("Server");
    d->loggerFactory = NULL;
    d->appPath = getAppPath();
    d->serverSocket = NULL;
    d->opcuaSessionCallback = new ServerPrivate::SessionCallback(*d);
    d->opcuaSession = NULL;
    d->messageIdCounter = 1;
    d->cachedConverterUa2ioCallback = NULL;
    d->converterUa2io = NULL;
    d->mutex = new Mutex(); // MutexException        
    d->isListening = false;
}

Server::~Server() /* throws HaSessionException, HaSubscriptionException */ {
    close(); // HaSessionException, HaSubscriptionException
    delete d->mutex;
    delete d->opcuaSessionCallback;
    delete[] d->appPath;
    delete d;
}

void Server::open(Server::Options& options) /* throws ServerException, ServerSocketException, HaSessionException */ {
    registerSignalHandler();
    // initialize the XML parser
    UaXmlDocument::initParser();
    // initialize the UA Stack platform layer    
    if (UaPlatformLayer::init() != 0) {
        std::ostringstream msg;
        msg << "Cannot initialize UA stack platform layer";
        throw ExceptionDef(ServerException, msg.str());
    }
    // load conf from file
    std::string serverConfDir(d->appPath);
    serverConfDir.append("/conf/");
    std::string serverConfFile("BinaryServerConfig.xml");
    d->log->info("Loading configuration file %s%s", serverConfDir.c_str(), serverConfFile.c_str());
    ConfigLoader configLoader(serverConfDir, serverConfFile);
    ConfigLoader::RemoteConfiguration remoteConf;
    ConfigLoader::LoggingConfiguration loggingConf;
    configLoader.load(d->serverConf, remoteConf, loggingConf);
    // set options to conf
    if (options.serverPort != NULL) {
        d->serverConf.port = *options.serverPort;
    }
    if (options.remoteHost != NULL) {
        remoteConf.host = *options.remoteHost;
    }
    if (options.remotePort != NULL) {
        remoteConf.port = *options.remotePort;
    }
    if (options.username != NULL) {
        remoteConf.username = options.username;
    }
    if (options.username != NULL) {
        remoteConf.password = options.password;
    }
    if (options.loggingFilePath != NULL) {
        // if absolute path
        if (options.loggingFilePath->at(0) == '/') {
            loggingConf.filePath = *options.loggingFilePath;
        } else {
            loggingConf.filePath = serverConfDir.append(*options.loggingFilePath);        
        }
    }
    // get session config
    d->sessionConf.host = remoteConf.host;
    d->sessionConf.port = remoteConf.port;
    d->sessionConf.username = remoteConf.username;
    d->sessionConf.password = remoteConf.password;
    d->sessionConf.connectTimeout = remoteConf.connectTimeout;
    d->sessionConf.sendReceiveTimeout = remoteConf.sendReceiveTimeout;
    d->sessionConf.maxReconnectDelay = remoteConf.maxReconnectDelay;
    d->sessionConf.publishingInterval = remoteConf.publishingInterval;
    // activate app logging
    UaTrace::TraceLevel uaAppTraceLevel = UaTrace::NoTrace;
    if (0 == loggingConf.uaAppLogLevel.compare("error")) {
        uaAppTraceLevel = UaTrace::Errors;
    } else if (0 == loggingConf.uaAppLogLevel.compare("warn")) {
        uaAppTraceLevel = UaTrace::Warning;
    } else if (0 == loggingConf.uaAppLogLevel.compare("info")) {
        uaAppTraceLevel = UaTrace::Info;
    } else if (0 == loggingConf.uaAppLogLevel.compare("debug")) {
        uaAppTraceLevel = UaTrace::CtorDtor; // incl. InterfaceCall
    } else if (0 == loggingConf.uaAppLogLevel.compare("trace")) {
        uaAppTraceLevel = UaTrace::Data; // incl. ProgramFlow
    }
    d->log->info("Activating logging to %s", loggingConf.filePath.c_str());
    LibT::initTrace(uaAppTraceLevel, 1000 /*maxTraceEntries*/, loggingConf.maxNumBackupFiles,
            UaString(loggingConf.filePath.c_str()), "uaclient");
    LibT::setTraceActive(uaAppTraceLevel != UaTrace::NoTrace);
    // activate stack logging
    OpcUa_UInt32 uaStackTraceLevel = OPCUA_TRACE_OUTPUT_LEVEL_NONE;
    if (0 == loggingConf.uaStackLogLevel.compare("error")) {
        uaStackTraceLevel = OPCUA_TRACE_OUTPUT_LEVEL_ERROR;
    } else if (0 == loggingConf.uaStackLogLevel.compare("warn")) {
        uaStackTraceLevel = OPCUA_TRACE_OUTPUT_LEVEL_WARNING;
    } else if (0 == loggingConf.uaStackLogLevel.compare("info")) {
        uaStackTraceLevel = OPCUA_TRACE_OUTPUT_LEVEL_INFO; // incl. System
    } else if (0 == loggingConf.uaStackLogLevel.compare("debug")) {
        uaStackTraceLevel = OPCUA_TRACE_OUTPUT_LEVEL_DEBUG;
    } else if (0 == loggingConf.uaStackLogLevel.compare("trace")) {
        uaStackTraceLevel = OPCUA_TRACE_OUTPUT_LEVEL_CONTENT;
    }
    UaPlatformLayer::changeTraceSettings(uaStackTraceLevel != OPCUA_TRACE_OUTPUT_LEVEL_NONE
            /*traceEnabled*/, uaStackTraceLevel);
    LibT::setStackTraceActive(uaStackTraceLevel != OPCUA_TRACE_OUTPUT_LEVEL_NONE);
    // replace default logger factory with factory for client SDK
    d->loggerFactory = new LoggerFactory(*new ClientSdkLoggerFactory(), true /*attachValues*/);
    d->log = LoggerFactory::getLogger("Server");
    // open a server socket
    ServerSocket* serverSocket = new ServerSocket(d->serverConf.acceptTimeout,
            d->serverConf.sendReceiveTimeout, true /*reuseAddress*/);
    ScopeGuard<ServerSocket> serverSocketSG(serverSocket);
    serverSocket->open(d->serverConf.port); // ServerSocketException
    // create a session to OPC UA server    
    HaSession* opcuaSession = new HaSession(d->sessionConf, *d->opcuaSessionCallback); // MutexException
    ScopeGuard<HaSession> opcuaSessionSG(opcuaSession);
    opcuaSession->open(); // HaSessionException        
    // the binary interface does not support namespaces => use the last loaded namespace
    d->namespaceIndex = opcuaSession->getNamespaceTable().length() - 1;
    // create converter incl. cache (OpcUa <-> IODataProvider)
    d->cachedConverterUa2ioCallback = new SASModelProviderNamespace::CachedConverterCallback(
            *new ServerPrivate::ConverterCallback(opcuaSession), true /*attachValues*/); //MutexException
    d->converterUa2io = new SASModelProviderNamespace::ConverterUa2IO(*d->cachedConverterUa2ioCallback);

    d->serverSocket = serverSocketSG.detach();
    d->opcuaSession = opcuaSessionSG.detach();
    // start the thread for processing the incoming messages via the binary interface
    d->isListening = true;
    pthread_create(&d->binaryThread, NULL, &ServerPrivate::binaryThreadRun, d);
}

void Server::close() /* throws HaSessionException, HaSubscriptionException */ {
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
    // close the server socket (avoids receiving of further messages in binary thread)
    if (d->serverSocket != NULL) {
        d->serverSocket->close();
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
    delete d->serverSocket;
    d->serverSocket = NULL;
    delete d->loggerFactory;
    d->loggerFactory = NULL;
    // clean up the UA Stack platform layer
    UaPlatformLayer::cleanup();
    // clean up the XML parser
    UaXmlDocument::cleanupParser();
}

void ServerPrivate::subscribe(Subscribe& request)
/*throws ConversionException, HaSubscriptionException, TimeoutException, 
 * ServerSocketException, MessageDeserializerException*/ {
    try {
        std::vector<const UaNodeId*>* nodeIds = new std::vector<const UaNodeId*>();
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
            cachedConverterUa2ioCallback->preload(*nodeAttributes[i]->getDataType());
        }
        // for each event type        
        for (std::map<UaNodeId, std::vector<BinaryServerNamespace::EventField*>*>::const_iterator it
                = eventFields.begin(); it != eventFields.end(); it++) {
            // preload event type infos
            cachedConverterUa2ioCallback->preload(it->first);
            std::vector<BinaryServerNamespace::EventField*>* eventFields = it->second;
            // for each event field of event type
            for (int i = 0; i < eventFields->size(); i++) {
                BinaryServerNamespace::EventField* eventField = (*eventFields)[i];
                // preload event field infos
                cachedConverterUa2ioCallback->preload(*eventField->getDataTypeId());
                delete eventField;
            }
            delete eventFields;
        }
    } catch (Exception& e) {
        // send response
        SubscribeResponse response(request.getMessageHeader().getMessageId(),
                Status::INVALID_PARAMETER);
        writeMessage(response); /* TimeoutException, ServerSocketException, MessageSerializerException */
        throw;
    }
    // send response
    SubscribeResponse response(request.getMessageHeader().getMessageId(), Status::SUCCESS);
    writeMessage(response); /* TimeoutException, ServerSocketException, MessageSerializerException */
}

void ServerPrivate::unsubscribe(Unsubscribe& request)
/*throws ConversionException, HaSubscriptionException, TimeoutException, 
 * ServerSocketException, MessageDeserializerException*/ {
    try {
        std::vector<const UaNodeId*>* nodeIds = new std::vector<const UaNodeId*>();
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
        writeMessage(response); /* TimeoutException, ServerSocketException, MessageSerializerException */
        throw;
    }
    // send response
    UnsubscribeResponse response(request.getMessageHeader().getMessageId(), Status::SUCCESS);
    writeMessage(response); /* TimeoutException, ServerSocketException, MessageSerializerException */
}

void ServerPrivate::read(Read& request)
/*throws ConversionException, HaSessionException, TimeoutException, 
 * ServerSocketException, MessageDeserializerException*/ {
    Variant* paramValue;
    try {
        std::vector<const UaNodeId*>* nodeIds = new std::vector<const UaNodeId*>();
        VectorScopeGuard<const UaNodeId> nodeIdsSG(nodeIds);
        // convert ParamId to UaNodeId
        UaNodeId* nodeId = convertBin2ua(request.getParamId()); // ConversionException
        nodeIds->push_back(nodeId);
        // read value incl. data type
        std::vector<NodeAttributes*>* values = new std::vector<NodeAttributes*>();
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

        ReadResponse response(request.getMessageHeader().getMessageId(), Status::INVALID_PARAMETER);
        writeMessage(response); /* TimeoutException, ServerSocketException, MessageSerializerException */
        throw;
    }
    // send response            
    ReadResponse response(request.getMessageHeader().getMessageId(), Status::SUCCESS,
            true /* attachValues */);
    response.setParamId(new ParamId(request.getParamId()));
    response.setParamValue(paramValue);
    writeMessage(response); /* TimeoutException, ServerSocketException, MessageSerializerException */
}

void ServerPrivate::write(Write& request)
/*throws ConversionException, HaSessionException, TimeoutException, 
 * ServerSocketException, MessageDeserializerException*/ {
    try {
        std::vector<const UaNodeId*>* nodeIds = new std::vector<const UaNodeId*>();
        VectorScopeGuard<const UaNodeId> nodeIdsSG(nodeIds);
        // convert ParamId to UaNodeId
        UaNodeId* nodeId = convertBin2ua(request.getParamId()); // ConversionException
        nodeIds->push_back(nodeId);
        // read data type
        std::vector<NodeAttributes*>* readValues = new std::vector<NodeAttributes*>();
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

        WriteResponse response(request.getMessageHeader().getMessageId(), Status::INVALID_PARAMETER);
        writeMessage(response); /* TimeoutException, ServerSocketException, MessageSerializerException */
        throw;
    }
    // send response
    WriteResponse response(request.getMessageHeader().getMessageId(), Status::SUCCESS);
    writeMessage(response); /* TimeoutException, ServerSocketException, MessageSerializerException */
}

void ServerPrivate::call(Call& request)
/*throws ConversionException, HaSessionException, TimeoutException, 
 * ServerSocketException, MessageDeserializerException*/ {
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
        const std::vector<const Variant*>& inputParams = request.getParamList().getElements();
        if (inputParams.size() != inputArgs.length()) {
            std::ostringstream msg;
            msg << "Invalid count of input arguments: server=" << inputArgs.length()
                    << ",caller=" << inputArgs.length();
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
        std::vector<UaVariant*>* outputValues = opcuaSession->call(*methodId, *objectId,
                *inputValues); // HaSessionException
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
            writeMessage(response); /* TimeoutException, ServerSocketException, MessageSerializerException */
        } else {
            CallResponse response(request.getMessageHeader().getMessageId(),
                    Status::INVALID_PARAMETER);
            writeMessage(response); /* TimeoutException, ServerSocketException, MessageSerializerException */
        }
        throw;
    }
    // send response
    CallResponse response(request.getMessageHeader().getMessageId(), Status::SUCCESS,
            true /* attachValues */);
    response.setMethodId(new ParamId(request.getMethodId()));
    response.setParamId(new ParamId(request.getParamId()));
    response.setParamList(new ParamList(*outputParams, true /*attachValues*/));
    writeMessage(response); /* TimeoutException, ServerSocketException, MessageSerializerException */
}

void ServerPrivate::dataChanged(std::vector<NodeAttributes*>& nodeAttributes)
/* throws Exception */ {
    ServerException* exception = NULL;
    std::map<const ParamId*, const Variant*>* elements
            = new std::map<const ParamId*, const Variant*>();
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
            *new ParamMap(*elements, true /*attachValues*/), true /*attachValues*/);
    if (elements->size() > 0) {
        try {
            writeMessage(notification); // TimeoutException, ServerSocketException, MessageSerializerException                                            
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

void ServerPrivate::newEvents(std::vector<const BinaryServerNamespace::Event*>& events)
/* throws Exception */ {
    ServerException* exception = NULL;
    // for each event
    for (int i = 0; i < events.size(); i++) {
        const BinaryServerNamespace::Event& event = *events[i];
        ParamId* eventTypeId = NULL;
        ParamId* paramId = NULL;
        long long timeStamp;
        OpcUa_UInt16 severity;
        const std::string* message = NULL;
        std::map<const ParamId*, const Variant*>* elements =
                new std::map<const ParamId*, const Variant*>();
        try {
            eventTypeId = convertUa2bin(event.getEventTypeId()); // ConversionException            
            std::vector<BinaryServerNamespace::EventField*>* eventFields = event.getEventFields();
            // for each event field
            for (int j = 0; j < eventFields->size(); j++) {
                BinaryServerNamespace::EventField& eventField = *(*eventFields)[j];
                if (eventField.getException() != NULL) {
                    throw *eventField.getException();
                }
                if (0 == eventField.getNodeId().namespaceIndex()) {
                    switch (eventField.getNodeId().identifierNumeric()) {
                        case OpcUaId_BaseEventType_SourceNode:
                        {
                            UaNodeId nodeId;
                            eventField.getValue()->toNodeId(nodeId);
                            paramId = convertUa2bin(nodeId); // ConversionException                            
                            break;
                        }
                        case OpcUaId_BaseEventType_Time:
                        case OpcUaId_BaseEventType_Severity:
                        case OpcUaId_BaseEventType_Message:
                        {
                            IODataProviderNamespace::Scalar* v =
                                    static_cast<IODataProviderNamespace::Scalar*> (
                                    converterUa2io->convertUa2io(*eventField.getValue(),
                                    *eventField.getDataTypeId())); // ConversionException
                            switch (eventField.getNodeId().identifierNumeric()) {
                                case OpcUaId_BaseEventType_Time:
                                    // received time stamp in 100 ns
                                    // 01.01.1601 - 01.01.1970: 11644473600 seconds
                                    timeStamp = v->getLLong() / 10 / 1000 - 11644473600000;
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
                    Variant* value = convertUa2bin(*eventField.getValue(), *eventField.getDataTypeId()); // ConversionException
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
            Event ev(messageIdCounter++, *eventTypeId, *paramId,
                    timeStamp, severity, *message, *new ParamMap(*elements, true /*attachValues*/),
                    true /*attachValues*/);
            writeMessage(ev); // TimeoutException, ServerSocketException, MessageSerializerException                 
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

Message* ServerPrivate::readMessage()
/* throws TimeoutException, ServerSocketException, MessageDeserializerException */ {
    unsigned char bufferHeader[BinaryMessageSerializer::MESSAGE_HEADER_LENGTH];
    serverSocket->readData(BinaryMessageSerializer::MESSAGE_HEADER_LENGTH,
            bufferHeader); // TimeoutException, ServerSocketException
    BinaryMessageDeserializer deserializer;
    MessageHeader* messageHeader = deserializer.deserializeMessageHeader(bufferHeader);
    ScopeGuard<MessageHeader> messageHeaderSG(messageHeader);
    size_t bufferSize = messageHeader->getMessageBodyLength();
    unsigned char bufferBody[bufferSize];
    serverSocket->readData(bufferSize, bufferBody); // TimeoutException, ServerSocketException

    return deserializer.deserializeMessageBody(*messageHeader,
            bufferBody); // MessageDeserializerException
}

void ServerPrivate::writeMessage(Message& message)
/* throws TimeoutException, ServerSocketException, MessageSerializerException */ {

    BinaryMessageSerializer serializer;
    unsigned char* responseBuffer = serializer.serialize(message); // MessageSerializerException
    ScopeGuard<unsigned char> responseBufferSG(responseBuffer, true /*isArray*/);
    serverSocket->writeData(BinaryMessageSerializer::MESSAGE_HEADER_LENGTH
            + message.getMessageHeader().getMessageBodyLength(),
            responseBuffer, false /*acceptConnection*/); // TimeoutException, ServerSocketException    
}

void* ServerPrivate::binaryThreadRun(void* object) {
    ServerPrivate& obj = *static_cast<ServerPrivate*> (object);
    // overtake conf
    int port;
    {
        MutexLock lock(*obj.mutex);
        port = obj.serverConf.port;
    }

    timespec reconnectDelay;
    reconnectDelay.tv_sec = 0;
    reconnectDelay.tv_nsec = 0;
    bool reopenServerSocket = false;

    bool isStopped = false;
    while (!isStopped) {
        try {
            if (reopenServerSocket) {
                // discard existing subscriptions
                obj.opcuaSession->unsubscribe(); // HaSubscriptionException
                // close the socket
                obj.serverSocket->close();
                // clear caches
                obj.cachedConverterUa2ioCallback->clear();
                // wait some time
                if (reconnectDelay.tv_sec < MAX_RECONNECT_DELAY) {
                    reconnectDelay.tv_sec++;
                }
                obj.log->info("Waiting %ld seconds...", reconnectDelay.tv_sec);
                nanosleep(&reconnectDelay, NULL /* remaining */);
                // reopen the server socket
                obj.serverSocket->open(port); // ServerSocketException
                reopenServerSocket = false;
            }
            Message* message = NULL;
            try {
                message = obj.readMessage(); // TimeoutException, ServerSocketException, MessageDeserializerException            
            } catch (TimeoutException& e) {
                // reading data from server socket is limited to be able to stop the server
            }
            if (message != NULL) {
                ScopeGuard<Message> messageSG(message);
                switch (message->getMessageHeader().getMessageType()) {
                    case MessageHeader::SUBSCRIBE:
                        obj.subscribe(*static_cast<Subscribe*> (message))
                                /*throws ConversionException, HaSubscriptionException, TimeoutException, 
                                 * ServerSocketException, MessageDeserializerException */;
                        break;
                    case MessageHeader::UNSUBSCRIBE:
                        obj.unsubscribe(*static_cast<Unsubscribe*> (message));
                        /*throws ConversionException, HaSubscriptionException, TimeoutException, 
                         * ServerSocketException, MessageDeserializerException */;
                        break;
                    case MessageHeader::READ:
                        obj.read(*static_cast<Read*> (message));
                        /*throws ConversionException, HaSessionException, TimeoutException, 
                         * ServerSocketException, MessageDeserializerException */;
                        break;
                    case MessageHeader::WRITE:
                        obj.write(*static_cast<Write*> (message));
                        /*throws ConversionException, HaSessionException, TimeoutException, 
                         * ServerSocketException, MessageDeserializerException */;
                        break;
                    case MessageHeader::CALL:
                        obj.call(*static_cast<Call*> (message));
                        /*throws ConversionException, HaSessionException, TimeoutException, 
                         * ServerSocketException, MessageDeserializerException */;
                        break;
                }
                // a message has been successfully processed => reset reconnect delay
                reconnectDelay.tv_sec = 0;
            }
        } catch (ServerSocketException& e) {
            // there is no way to inform the OPC UA server => log the exception
            std::string st;
            e.getStackTrace(st);
            obj.log->error("Exception while processing received binary message: %s", st.c_str());
            reopenServerSocket = true;
        } catch (Exception& e) {
            // there is no way to inform the OPC UA server => log the exception
            std::string st;
            e.getStackTrace(st);
            obj.log->error("Exception while processing received binary message: %s", st.c_str());
        }
        MutexLock lock(*obj.mutex);
        isStopped = !obj.isListening;
    }
    obj.log->info("Stopping thread for receiving binary messages");

    return NULL;
}

UaNodeId* ServerPrivate::convertBin2ua(const ParamId& paramId) /* throws ConversionException */ {
    ConverterBin2IO converterIO2bin;
    IODataProviderNamespace::NodeId* nid = converterIO2bin.convertBin2io(paramId, namespaceIndex); // ConversionException
    ScopeGuard<IODataProviderNamespace::NodeId> nidSG(nid);

    return converterUa2io->convertIo2ua(*nid); // ConversionException
}

ParamId* ServerPrivate::convertUa2bin(const UaNodeId& nodeId) /* throws ConversionException */ {
    IODataProviderNamespace::NodeId* nid = converterUa2io->convertUa2io(nodeId); // ConversionException
    ScopeGuard<IODataProviderNamespace::NodeId> nidSG(nid);
    ConverterBin2IO converterIo2bin;

    return converterIo2bin.convertIo2bin(*nid); // ConversionException
}

UaVariant* ServerPrivate::convertBin2ua(const Variant& value, const UaNodeId& destDataTypeId)
/* throws ConversionException */ {
    ConverterBin2IO converterIo2bin;
    IODataProviderNamespace::Variant* v = converterIo2bin.convertBin2io(value, namespaceIndex); // ConversionException
    ScopeGuard<IODataProviderNamespace::Variant> vSG(v);

    return converterUa2io->convertIo2ua(*v, destDataTypeId); // ConversionException
}

Variant* ServerPrivate::convertUa2bin(const UaVariant& value, const UaNodeId& srcDataTypeId)
/*throws ConversionException*/ {
    IODataProviderNamespace::Variant* v = converterUa2io->convertUa2io(value, srcDataTypeId); // ConversionException
    ScopeGuard<IODataProviderNamespace::Variant> vSG(v);
    ConverterBin2IO converterIo2bin;
    return converterIo2bin.convertIo2bin(*v); // ConversionException
}
