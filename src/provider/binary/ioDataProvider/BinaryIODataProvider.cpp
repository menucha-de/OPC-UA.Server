#include "BinaryIODataProvider.h"
#include "../common/ClientSocket.h"
#include "../common/ClientSocketException.h"
#include "../common/ConversionException.h"
#include "../common/TimeoutException.h"
#include "../messages/BinaryMessageDeserializer.h"
#include "../messages/BinaryMessageSerializer.h"
#include "../messages/ConverterBin2IO.h"
#include "../messages/MessageQueue.h"
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
#include <libxml/parser.h> // xmlReadFile
#include <libxml/encoding.h> // xmlGetCharEncodingName
#include <pthread.h> // pthread_t
#include <sstream> // std::ostringstream
#include <string>
#include <time.h> // nanosleep
#include <ctime> // std::time_t
#ifdef DEBUG
#include <CppUTest/MemoryLeakDetectorNewMacros.h>
#endif

using namespace CommonNamespace;

class BinaryIODataProviderPrivate {
    friend class BinaryIODataProvider;
private:

    class CallbackData {
    public:
        const IODataProviderNamespace::NodeId* nodeId;
        IODataProviderNamespace::SubscriberCallback* callback;
    };

    Logger* log;

    bool unitTesting;
    int namespaceIndex;
    ClientSocket* clientSocket;
    MessageQueue* incomingQueue;
    pthread_t incomingQueueThread;
    int maxIncomingMessageQueueSize;
    int connectTimeout; // in sec.
    ConverterBin2IO converter;

    Mutex* mutex;
    std::string host;
    int port;
    int sendReceiveTimeout; // in sec.
    int maxReconnectDelay; // in sec.    
    int incomingMessageExpirationTime; // in sec.
    unsigned long messageIdCounter;
    bool isListening;
    std::vector<CallbackData*> callbacks;

    static void* incomingQueueThreadRun(void* object);
    static void* renewSubscriptionsThreadRun(void* object);
    void sendMessage(Message& message);
    void renewSubscriptions() /* throws IODataProviderException */;

    void loadConfig(const std::string& confFile);
    void parseConfig(const xmlNode* startNode);
};

BinaryIODataProvider::BinaryIODataProvider(bool unitTesting) /* throws MutexException */ {
    d = new BinaryIODataProviderPrivate();
    d->log = LoggerFactory::getLogger("BinaryIODataProvider");
    d->unitTesting = unitTesting;
    d->clientSocket = NULL;
    d->mutex = new Mutex(); // MutexException
    d->messageIdCounter = 1;
    d->isListening = false;
}

BinaryIODataProvider::~BinaryIODataProvider() {
    close();
    delete d->mutex;
    delete d;
}

void BinaryIODataProvider::open(const std::string& confDir) /* throws IODataProviderException */ {
    std::string confFile = confDir + "config.xml";
    try {
        d->log->info("Opening BinaryIODataProvider with configuration directory %s",
                confDir.c_str());
        // load config
        d->log->info("Loading configuration file %s", confFile.c_str());
        d->loadConfig(confFile);
        // create a client socket
        ClientSocket* clientSocket = new ClientSocket(d->connectTimeout, d->sendReceiveTimeout); // MutexException
        ScopeGuard<ClientSocket> clientSocketSG(clientSocket);
        timespec reconnectDelay;
        reconnectDelay.tv_sec = 0;
        reconnectDelay.tv_nsec = 0;
        std::time_t time = std::time(NULL);
        std::time_t endTime = time + d->connectTimeout;
        std::time_t remainingTimeout = endTime - time;
        bool isOpened = false;
        while (!isOpened) {
            if (remainingTimeout <= 0) {
                std::ostringstream msg;
                msg << "Time out after " << d->connectTimeout << " sec.";
                throw ExceptionDef(TimeoutException, msg.str());
            }
            clientSocket->setConnectTimeout(remainingTimeout);
            try {
                // open a connection                
                clientSocket->open(d->host, d->port); // TimeoutException, ClientSocketException
                isOpened = true;
            } catch (ClientSocketException& e) {
                // there is no way to inform the OPC UA server => log the exception
                std::string st;
                e.getStackTrace(st);
                d->log->error("Exception while opening IO data provider: %s", st.c_str());
                // close the connection
                clientSocket->close();
                // wait some time
                if (reconnectDelay.tv_sec < d->maxReconnectDelay) {
                    reconnectDelay.tv_sec++;
                }
                remainingTimeout = endTime - std::time(NULL);
                if (reconnectDelay.tv_sec > remainingTimeout) {
                    reconnectDelay.tv_sec = remainingTimeout;
                }
                d->log->info("Waiting %ld seconds...", reconnectDelay.tv_sec);
                nanosleep(&reconnectDelay, NULL /* remaining */);
                remainingTimeout -= reconnectDelay.tv_sec;
            }
        }
        // create queue for incoming messages via binary interface
        d->incomingQueue = new MessageQueue(d->maxIncomingMessageQueueSize,
                true /*attachValues*/); // MutexException

        d->clientSocket = clientSocketSG.detach();
        // start the thread for processing the incoming messages via the binary interface
        d->isListening = true;
        pthread_create(&d->incomingQueueThread, NULL,
                &BinaryIODataProviderPrivate::incomingQueueThreadRun, d);
    } catch (Exception& e) {
        // convert exception to IODataProviderException
        std::ostringstream msg;
        msg << "Cannot open IO data provider using configuration " << confFile;
        IODataProviderNamespace::IODataProviderException ex = ExceptionDef(
                IODataProviderNamespace::IODataProviderException, msg.str());
        ex.setCause(&e);
        throw ex;
    }
}

void BinaryIODataProvider::close() {
    //TODO call "unsubscribe" for existing callbacks
    // set flag for stopping the thread
    bool isListening;
    {
        MutexLock lock(*d->mutex);
        isListening = d->isListening;
        d->isListening = false;
    }
    // close the client socket (avoids further usage in thread)
    if (d->clientSocket != NULL) {
        d->log->info("Closing BinaryIODataProvider");
        d->clientSocket->close();
    }
    if (isListening) {
        // wait for end of thread
        pthread_join(d->incomingQueueThread, NULL /*return*/);
        // delete message queue
        delete d->incomingQueue;
    }
    // delete client socket
    if (d->clientSocket != NULL) {
        delete d->clientSocket;
        d->clientSocket = NULL;
    }
}

const IODataProviderNamespace::NodeProperties* BinaryIODataProvider::getDefaultNodeProperties(
        const std::string& namespaceUri, int namespaceIndex) {
    // the interface to the server part does not support namespaces => use the last one
    d->namespaceIndex = namespaceIndex;
    //TODO get supported namespaces and their value handling from conf
    return new IODataProviderNamespace::NodeProperties(
            IODataProviderNamespace::NodeProperties::ASYNC);
}

std::vector<const IODataProviderNamespace::NodeData*>* BinaryIODataProvider::getNodeProperties(
        const std::string& namespaceUri, int namespaceIndex) {
    //TODO get nodes data from conf
    std::vector<const IODataProviderNamespace::NodeData*>* ret =
            new std::vector<const IODataProviderNamespace::NodeData*>();
    int idsNoneCount = 12;
    const char* idsNone[idsNoneCount] = {"AirConditioner_1.Humidity.EURange",
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
        "Furnace_2.TemperatureSetpoint.EngineeringUnits"};
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
    const char* idsSync[idsSyncCount] = {        
        "rfr310.IOData.HS1",
        "rfr310.IOData.HS1.direction",
        "rfr310.IOData.HS2",
        "rfr310.IOData.HS2.direction",
        "rfr310.IOData.HS3",
        "rfr310.IOData.HS3.direction",
        "rfr310.IOData.HS4",
        "rfr310.IOData.HS4.direction",
        "rfr310.IOData.SWS1_SWD1",
        "rfr310.IOData.SWS1_SWD1.direction",
        "rfr310.IOData.SWS2_SWD2",
        "rfr310.IOData.SWS2_SWD2.direction",
        "rfr310.IOData.LS1",
        "rfr310.IOData.LS1.direction",
        "rfr310.IOData.LS2",
        "rfr310.IOData.LS2.direction",
        "rfr310.RuntimeParameters.RfPower",
        "rfr310.RuntimeParameters.MinRssi",
        "AirConditioner_1.Temperature",
        "ControllerConfigurations"
    };
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

std::vector<IODataProviderNamespace::NodeData*>* BinaryIODataProvider::read(
        const std::vector<const IODataProviderNamespace::NodeId*>& nodeIds) /* throws IODataProviderException */ {
    unsigned long messageId;
    int sendReceiveTimeout;
    int namespaceIndex;
    {
        MutexLock lock(*d->mutex);
        messageId = d->messageIdCounter++;
        sendReceiveTimeout = d->sendReceiveTimeout;
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
            // create paramId
            ParamId* paramId = d->converter.convertIo2bin(nodeId); // ConversionException
            // create request                        
            Read request(messageId, *paramId, true /* attachValues */);
            // send request
            d->sendMessage(request); // ClientSocketException
            // get response
            ReadResponse* readResponse = static_cast<ReadResponse*> (
                    d->incomingQueue->get(messageId, sendReceiveTimeout)); // TimeoutException
            ScopeGuard<ReadResponse> readResponseSG(readResponse);
            if (readResponse->getStatus() == Status::SUCCESS) {
                // convert param value
                nodeValue = d->converter.convertBin2io(
                        *readResponse->getParamValue(), namespaceIndex); // ConversionException                
            } else {
                std::ostringstream msg;
                msg << "Received a read response for " << nodeId.toString().c_str()
                        << ": messageId=" << messageId
                        << ",status=" << readResponse->getStatus();
                exception = new ExceptionDef(IODataProviderNamespace::IODataProviderException,
                        msg.str());
            }
        } catch (Exception& e) {
            exception = new ExceptionDef(IODataProviderNamespace::IODataProviderException,
                    std::string("Cannot read data for ").append(nodeId.toString()));
            exception->setCause(&e);
        }
        // add value to result list
        IODataProviderNamespace::NodeData* nodeData = new IODataProviderNamespace::NodeData(
                *new IODataProviderNamespace::NodeId(nodeId), nodeValue,
                true /* attachValues */);
        nodeData->setException(exception);
        ret->push_back(nodeData);
        MutexLock lock(*d->mutex);
        messageId = d->messageIdCounter++;
    }
    return retSG.detach();
}

void BinaryIODataProvider::write(
        const std::vector<const IODataProviderNamespace::NodeData*>& nodeData,
        bool sendValueChangedEvents) /* throws IODataProviderException */ {
    unsigned long messageId;
    int sendReceiveTimeout;
    {
        MutexLock lock(*d->mutex);
        messageId = d->messageIdCounter++;
        sendReceiveTimeout = d->sendReceiveTimeout;
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
                exception = new ExceptionDef(IODataProviderNamespace::IODataProviderException,
                        std::string("Cannot write a NULL value for ").append(nodeId.toString()));
                // continue with next node
                continue;
            }
            Variant* paramValue = d->converter.convertIo2bin(*nodeValue); // ConversionException
            // create request            
            Write request(messageId, *paramIdSG.detach(), *paramValue, true /* attachValues */);
            // send request
            d->sendMessage(request); // ClientSocketException            
            // get response
            WriteResponse* writeResponse = static_cast<WriteResponse*> (
                    d->incomingQueue->get(messageId, sendReceiveTimeout)); // TimeoutException
            ScopeGuard<WriteResponse> writeResponseSG(writeResponse);
            if (writeResponse->getStatus() != Status::SUCCESS && exception == NULL) {
                std::ostringstream msg;
                msg << "Received a write response for " << nodeId.toString().c_str()
                        << ": messageId=" << messageId
                        << ",status=" << writeResponse->getStatus();
                exception = new ExceptionDef(IODataProviderNamespace::IODataProviderException,
                        msg.str());
            }
            MutexLock lock(*d->mutex);
            messageId = d->messageIdCounter++;
        } catch (Exception& e) {
            if (exception == NULL) {
                exception = new ExceptionDef(IODataProviderNamespace::IODataProviderException,
                        std::string("Cannot write data for nodeId ").append(nodeId.toString()));
                exception->setCause(&e);
            }
        }
    }
    if (exception != NULL) {
        IODataProviderNamespace::IODataProviderException ex =
                ExceptionDef(IODataProviderNamespace::IODataProviderException,
                std::string("Cannot write data"));
        ex.setCause(exception);
        delete exception;
        throw ex;
    }
}

std::vector<IODataProviderNamespace::MethodData*>* BinaryIODataProvider::call(
        const std::vector<const IODataProviderNamespace::MethodData*>& methodData) {
    std::vector<IODataProviderNamespace::MethodData*>* ret =
            new std::vector<IODataProviderNamespace::MethodData*>();
    VectorScopeGuard<IODataProviderNamespace::MethodData> retSG(ret);
    unsigned long messageId;
    int sendReceiveTimeout;
    int namespaceIndex;
    {
        MutexLock lock(*d->mutex);
        messageId = d->messageIdCounter++;
        sendReceiveTimeout = d->sendReceiveTimeout;
        namespaceIndex = d->namespaceIndex;
    }
    // for each method
    for (int i = 0; i < methodData.size(); i++) {
        const IODataProviderNamespace::MethodData& methodDataElem = *methodData[i];
        std::vector<const IODataProviderNamespace::Variant*>* outputArgs =
                new std::vector<const IODataProviderNamespace::Variant*>();
        VectorScopeGuard<const IODataProviderNamespace::Variant> outputArgsSG(outputArgs);
        IODataProviderNamespace::IODataProviderException* exception = NULL;
        try {
            // convert NodeId to ParamId
            ParamId* methodId = d->converter.convertIo2bin(
                    methodDataElem.getMethodNodeId()); // ConversionException
            ScopeGuard<ParamId> methodIdSG(methodId);
            ParamId* objectId = d->converter.convertIo2bin(
                    methodDataElem.getObjectNodeId()); // ConversionException
            ScopeGuard<ParamId> objectIdSG(objectId);
            std::vector<const Variant*>* inputArgs = new std::vector<const Variant*>();
            ScopeGuard<ParamList> inputArgsSG(new ParamList(*inputArgs, true /*attachValues*/));
            // for each method input argument        
            for (int j = 0; j < methodDataElem.getMethodArguments().size(); j++) {
                Variant* value = d->converter.convertIo2bin(
                        *methodDataElem.getMethodArguments()[j]); // ConversionException
                inputArgs->push_back(value);
            }
            // create request            
            Call request(messageId, *methodIdSG.detach(), *objectIdSG.detach(),
                    *inputArgsSG.detach(), true /* attachValues */);
            // send request
            d->sendMessage(request); // ClientSocketException                 
            // get response
            CallResponse* callResponse = static_cast<CallResponse*> (
                    d->incomingQueue->get(messageId, sendReceiveTimeout)); // TimeoutException
            ScopeGuard<CallResponse> callResponseSG(callResponse);
            switch (callResponse->getStatus()) {
                case Status::SUCCESS:
                {
                    const std::vector<const Variant*>& params =
                            callResponse->getParamList()->getElements();
                    // for each method output argument
                    for (int j = 0; j < params.size(); j++) {
                        // convert Variant (binary) to Variant (IODataProvider)
                        IODataProviderNamespace::Variant* value = d->converter.convertBin2io(
                                *params[j], namespaceIndex); // ConversionException
                        // add value to output arguments
                        outputArgs->push_back(value);
                    }
                    break;
                }
                case Status::APPLICATION_ERROR:
                {
                    const std::vector<const Variant*>& params =
                            callResponse->getParamList()->getElements();
                    unsigned long errorCode = static_cast<unsigned long> (
                            static_cast<const Scalar*> (params[0])->getInt());
                    std::ostringstream msg;
                    msg << "Received a call response with application error 0x" << std::hex << errorCode;
                    exception = new ExceptionDef(IODataProviderNamespace::IODataProviderException,
                            msg.str());
                    exception->setErrorCode(&errorCode);
                    break;
                }
                default:
                    std::ostringstream msg;
                    msg << "Received a call response with status " << callResponse->getStatus();
                    exception = new ExceptionDef(IODataProviderNamespace::IODataProviderException,
                            msg.str());
            }
        } catch (Exception& e) {
            exception = new ExceptionDef(IODataProviderNamespace::IODataProviderException,
                    std::string("Cannot call method ")
                    .append(methodDataElem.getMethodNodeId().toString())
                    .append(" on object ")
                    .append(methodDataElem.getObjectNodeId().toString()));
            exception->setCause(&e);
        }
        IODataProviderNamespace::MethodData* md = new IODataProviderNamespace::MethodData(
                *new IODataProviderNamespace::NodeId(methodDataElem.getObjectNodeId()),
                *new IODataProviderNamespace::NodeId(methodDataElem.getMethodNodeId()),
                *outputArgsSG.detach(), true /*attachValues*/);
        md->setException(exception);
        ret->push_back(md);
        MutexLock lock(*d->mutex);
        messageId = d->messageIdCounter++;
    }
    return retSG.detach();
}

std::vector<IODataProviderNamespace::NodeData*>* BinaryIODataProvider::subscribe(
        const std::vector<const IODataProviderNamespace::NodeId*>& nodeIds,
        IODataProviderNamespace::SubscriberCallback& callback) /* throws IODataProviderException */ {
    unsigned long messageId;
    int sendReceiveTimeout;
    {
        MutexLock lock(*d->mutex);
        messageId = d->messageIdCounter++;
        sendReceiveTimeout = d->sendReceiveTimeout;
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
            d->sendMessage(request); // ClientSocketException
            // get response
            SubscribeResponse* subscribeResponse = static_cast<SubscribeResponse*> (
                    d->incomingQueue->get(messageId, sendReceiveTimeout)); // TimeoutException
            ScopeGuard<SubscribeResponse> subscribeResponseSG(subscribeResponse);
            if (subscribeResponse->getStatus() != Status::SUCCESS) {
                std::ostringstream msg;
                msg << "Received a subscribe response for " << nodeId.toString().c_str()
                        << ": messageId=" << messageId
                        << ",status=" << subscribeResponse->getStatus();
                exception = new ExceptionDef(IODataProviderNamespace::IODataProviderException,
                        msg.str());
            }
        } catch (Exception& e) {
            exception = new ExceptionDef(IODataProviderNamespace::IODataProviderException,
                    std::string("Cannot create a subscription for ").append(nodeId.toString()));
            exception->setCause(&e);
        }
        exceptions.push_back(exception);
        MutexLock lock(*d->mutex);
        if (exception == NULL) {
            // add callback to list
            BinaryIODataProviderPrivate::CallbackData* callbackData =
                    new BinaryIODataProviderPrivate::CallbackData();
            callbackData->nodeId = new IODataProviderNamespace::NodeId(nodeId);
            callbackData->callback = &callback;
            d->callbacks.push_back(callbackData);
        }
        // get new messageId
        messageId = d->messageIdCounter++;
    }
    // set exceptions to read results
    std::vector<IODataProviderNamespace::NodeData*>* readResults = read(nodeIds);
    for (int i = 0; i < exceptions.size(); i++) {
        IODataProviderNamespace::IODataProviderException* exception = exceptions[i];
        if (exception != NULL) {
            (*readResults)[i]->setException(exception);
        }
    }
    return readResults;
}

void BinaryIODataProvider::unsubscribe(
        const std::vector<const IODataProviderNamespace::NodeId*>& nodeIds) /* throws IODataProviderException */ {
    unsigned long messageId;
    int sendReceiveTimeout;
    {
        MutexLock lock(*d->mutex);
        messageId = d->messageIdCounter++;
        sendReceiveTimeout = d->sendReceiveTimeout;
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
            d->sendMessage(request); // ClientSocketException
            // get response
            UnsubscribeResponse* unsubscribeResponse = static_cast<UnsubscribeResponse*> (
                    d->incomingQueue->get(messageId, sendReceiveTimeout)); // TimeoutException
            ScopeGuard<UnsubscribeResponse> unsubscribeResponseSG(unsubscribeResponse);
            if (unsubscribeResponse->getStatus() != Status::SUCCESS && exception == NULL) {
                std::ostringstream msg;
                msg << "Received an unsubscribe response for " << nodeId.toString().c_str()
                        << ": messageId=" << messageId
                        << ",status=" << unsubscribeResponse->getStatus();
                exception = new ExceptionDef(IODataProviderNamespace::IODataProviderException,
                        msg.str());
            }
        } catch (Exception& e) {
            if (exception == NULL) {
                // convert exception to IODataProviderException
                exception = new ExceptionDef(IODataProviderNamespace::IODataProviderException,
                        std::string("Cannot delete subscription for ").append(nodeId.toString()));
                exception->setCause(&e);
            }
        }
        MutexLock lock(*d->mutex);
        if (exception == NULL) {
            // for each callback data
            std::vector<BinaryIODataProviderPrivate::CallbackData*>::iterator callbackDataIter
                    = d->callbacks.begin();
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
        IODataProviderNamespace::IODataProviderException ex =
                ExceptionDef(IODataProviderNamespace::IODataProviderException,
                std::string("Deletion of subscriptions failed"));
        ex.setCause(exception);
        delete exception;
        throw ex;
    }
}

void BinaryIODataProviderPrivate::renewSubscriptions() /* throws IODataProviderException */ {
    unsigned long messageId;
    int currSendReceiveTimeout;
    {
        MutexLock lock(*mutex);
        messageId = messageIdCounter++;
        currSendReceiveTimeout = sendReceiveTimeout;
    }
    IODataProviderNamespace::IODataProviderException* exception = NULL;
    // for each node in reverse order
    for (int i = callbacks.size() - 1; i >= 0; i--) {
        const IODataProviderNamespace::NodeId& nodeId = *(callbacks[i])->nodeId;
        try {
            // create paramId
            ParamId* paramId = converter.convertIo2bin(nodeId); // ConversionException
            // create unsubscribe request
            Unsubscribe unsubscribeRequest(messageId, *paramId, true /*attachValues*/);
            // send request
            sendMessage(unsubscribeRequest); // ClientSocketException
            bool subscribe = true;
            // get response
            UnsubscribeResponse* unsubscribeResponse = static_cast<UnsubscribeResponse*> (
                    incomingQueue->get(messageId, currSendReceiveTimeout)); // TimeoutException
            ScopeGuard<UnsubscribeResponse> unsubscribeResponseSG(unsubscribeResponse);
            if (unsubscribeResponse->getStatus() != Status::SUCCESS) {
                if (exception == NULL) {
                    std::ostringstream msg;
                    msg << "Received an unsubscribe response for " << nodeId.toString().c_str()
                            << ": messageId=" << messageId
                            << ",status=" << unsubscribeResponse->getStatus();
                    exception = new ExceptionDef(IODataProviderNamespace::IODataProviderException,
                            msg.str());
                }
                subscribe = false;
            }
            {
                MutexLock lock(*mutex);
                messageId = messageIdCounter++;
            }
            if (subscribe) {
                // create subscribe request            
                Subscribe subscribeRequest(messageId, *paramId);
                // send request
                sendMessage(subscribeRequest); // ClientSocketException            
                // get response
                SubscribeResponse* subscribeResponse = static_cast<SubscribeResponse*> (
                        incomingQueue->get(messageId, currSendReceiveTimeout)); // TimeoutException
                ScopeGuard<SubscribeResponse> subscribeResponseSG(subscribeResponse);
                if (subscribeResponse->getStatus() != Status::SUCCESS && exception == NULL) {
                    std::ostringstream msg;
                    msg << "Received a subscribe response for " << nodeId.toString().c_str()
                            << ": messageId=" << messageId
                            << ",status=" << subscribeResponse->getStatus();
                    exception = new ExceptionDef(IODataProviderNamespace::IODataProviderException,
                            msg.str());
                }
                MutexLock lock(*mutex);
                messageId = messageIdCounter++;
            }
        } catch (Exception& e) {
            if (exception == NULL) {
                exception = new ExceptionDef(IODataProviderNamespace::IODataProviderException,
                        std::string("Cannot recreate subscription for ").append(nodeId.toString()));
                exception->setCause(&e);
            }
        }
    } // for each node
    if (exception != NULL) {
        IODataProviderNamespace::IODataProviderException ex =
                ExceptionDef(IODataProviderNamespace::IODataProviderException,
                std::string("Cannot recreate subscriptions"));
        ex.setCause(exception);
        delete exception;
        throw ex;
    }
}

void* BinaryIODataProviderPrivate::renewSubscriptionsThreadRun(void* object) {
    BinaryIODataProviderPrivate& obj = *static_cast<BinaryIODataProviderPrivate*> (object);
    try {
        obj.renewSubscriptions(); // IODataProviderException
    } catch (Exception& e) {
        // there is no way to inform the OPC UA server => log the exception
        std::string st;
        e.getStackTrace(st);
        obj.log->error("Exception while renewing subscriptions: %s", st.c_str());
    }
}

void* BinaryIODataProviderPrivate::incomingQueueThreadRun(void* object) {
    BinaryIODataProviderPrivate& obj = *static_cast<BinaryIODataProviderPrivate*> (object);
    obj.log->info("Started thread for receiving messages");
    // overtake conf
    std::string host;
    int port;
    int maxReconnectDelay;
    int incomingMessageExpirationTime;
    {
        MutexLock lock(*obj.mutex);
        host = obj.host;
        port = obj.port;
        maxReconnectDelay = obj.maxReconnectDelay;
        incomingMessageExpirationTime = obj.incomingMessageExpirationTime;
    }

    timespec reconnectDelay;
    reconnectDelay.tv_sec = 0;
    reconnectDelay.tv_nsec = 0;

    pthread_t renewSubscriptionsThread = 0;
    bool reopenClientSocket = false;

    bool isStopped = false;
    while (!isStopped) {
        try {
            if (reopenClientSocket) {
                // close the connection
                obj.clientSocket->close();
                // wait some time
                if (reconnectDelay.tv_sec < maxReconnectDelay) {
                    reconnectDelay.tv_sec++;
                }
                obj.log->info("Waiting %ld seconds...", reconnectDelay.tv_sec);
                nanosleep(&reconnectDelay, NULL /* remaining */);
                // ensure the previous thread for renewal of the subscriptions is stopped
                pthread_join(renewSubscriptionsThread, NULL /*return*/);
                // reopen the connection
                obj.clientSocket->open(host, port); // ClientSocketException
                // start the renewal of the subscriptions in a separate thread
                // (the incoming responses must be processed in the current thread)
                pthread_create(&renewSubscriptionsThread, NULL,
                        &BinaryIODataProviderPrivate::renewSubscriptionsThreadRun, object);
                reopenClientSocket = false;
            }
            unsigned char bufferHeader[BinaryMessageSerializer::MESSAGE_HEADER_LENGTH];
            // read message header
            obj.clientSocket->readData(
                    BinaryMessageSerializer::MESSAGE_HEADER_LENGTH,
                    bufferHeader); // TimeoutException, ClientSocketException
            // deserialize message header
            BinaryMessageDeserializer deserializer;
            MessageHeader* messageHeader =
                    deserializer.deserializeMessageHeader(bufferHeader);
            ScopeGuard<MessageHeader> messageHeaderSG(messageHeader);
            size_t bufferSize = messageHeader->getMessageBodyLength();
            unsigned char bufferBody[bufferSize];
            // read message body
            obj.clientSocket->readData(bufferSize, bufferBody); // TimeoutException, ClientSocketException
            // a message was read successfully => reset reconnect delay
            reconnectDelay.tv_sec = 0;

            // deserialize message body
            Message* message = deserializer.deserializeMessageBody(
                    *messageHeader, bufferBody); // MessageDeserializerException
            ScopeGuard<Message> messageSG(message);
            // if notification or OPC UA event
            switch (message->getMessageHeader().getMessageType()) {
                case MessageHeader::NOTIFICATION:
                {
                    Notification* notification = static_cast<Notification*> (message);
                    int namespaceIndex;
                    {
                        MutexLock lock(*obj.mutex);
                        namespaceIndex = obj.namespaceIndex;
                    }
                    const std::map<const ParamId*, const Variant*>& elements =
                            notification->getParamMap().getElements();
                    // for each param
                    for (std::map<const ParamId*, const Variant*>::const_iterator i =
                            elements.begin(); i != elements.end(); i++) {
                        const ParamId& paramId = *(*i).first;
                        const Variant& paramValue = *(*i).second;
                        // nodeId
                        IODataProviderNamespace::NodeId* ioNodeId =
                                obj.converter.convertBin2io(paramId, namespaceIndex); // ConversionException
                        ScopeGuard<IODataProviderNamespace::NodeId> ioNodeIdSG(ioNodeId);
                        // nodeValue
                        IODataProviderNamespace::Variant* ioNodeValue =
                                obj.converter.convertBin2io(paramValue, namespaceIndex); // ConversionException                        
                        // nodeData incl nodeId, nodeValue
                        std::vector<const IODataProviderNamespace::NodeData*>* ioNodeData =
                                new std::vector<
                                const IODataProviderNamespace::NodeData*>();
                        ioNodeData->push_back(
                                new IODataProviderNamespace::NodeData(
                                *ioNodeIdSG.detach(), ioNodeValue,
                                true /* attachValues */));
                        // dateTime
                        // TODO get timestamp from notification message
                        // event incl. dateTime, nodeData
                        IODataProviderNamespace::Event ioEvent(time(NULL) * 1000, *ioNodeData,
                                true /* attachValues */);
                        // send IO data provider event via callbacks
                        for (int i = 0; i < obj.callbacks.size(); i++) {
                            CallbackData& callbackData = *obj.callbacks[i];
                            if (callbackData.nodeId->equals(*ioNodeId)) {
                                callbackData.callback->valuesChanged(ioEvent); // SubscriberCallbackException
                            }
                        }
                    }
                    break;
                }
                case MessageHeader::EVENT:
                {
                    Event* event = static_cast<Event*> (message);
                    int namespaceIndex;
                    {
                        MutexLock lock(*obj.mutex);
                        namespaceIndex = obj.namespaceIndex;
                    }
                    // sourceNodeId
                    ScopeGuard<IODataProviderNamespace::NodeId> ioSourceNodeIdSG(
                            obj.converter.convertBin2io(event->getParamId(), namespaceIndex)); // ConversionException
                    // fieldData
                    std::vector<const IODataProviderNamespace::NodeData*>* ioFieldData
                            = new std::vector<const IODataProviderNamespace::NodeData*>();
                    VectorScopeGuard<const IODataProviderNamespace::NodeData> ioFieldDataSG(ioFieldData);
                    const std::map<const ParamId*, const Variant*>& elems =
                            event->getParamMap().getElements();
                    for (std::map<const ParamId*, const Variant*>::const_iterator i = elems.begin();
                            i != elems.end(); i++) {
                        const ParamId& paramId = *(*i).first;
                        const Variant& paramValue = *(*i).second;
                        ScopeGuard<IODataProviderNamespace::NodeId> nodeIdSG(
                                obj.converter.convertBin2io(paramId, namespaceIndex)); // ConversionException
                        IODataProviderNamespace::Variant* nodeValue =
                                obj.converter.convertBin2io(paramValue, namespaceIndex); // ConversionException
                        ioFieldData->push_back(new IODataProviderNamespace::NodeData(
                                *nodeIdSG.detach(), nodeValue, true /*attachValue*/));
                    }
                    // eventValue incl. sourceNodeId, message, severity, fieldData
                    IODataProviderNamespace::OpcUaEventData* ioEventValue =
                            new IODataProviderNamespace::OpcUaEventData(*ioSourceNodeIdSG.detach(),
                            *new std::string(event->getMessage()), event->getSeverity(),
                            *ioFieldDataSG.detach(), true /* attachValues*/);
                    ScopeGuard<IODataProviderNamespace::OpcUaEventData> ioEventValueSG(ioEventValue);
                    // eventTypeId
                    IODataProviderNamespace::NodeId* ioEventTypeId =
                            obj.converter.convertBin2io(event->getEventId(), namespaceIndex); // ConversionException
                    // eventData incl. eventTypeId, eventValue
                    std::vector<const IODataProviderNamespace::NodeData*>* ioEventData =
                            new std::vector<const IODataProviderNamespace::NodeData*>();
                    ioEventData->push_back(new IODataProviderNamespace::NodeData(
                            *ioEventTypeId, ioEventValueSG.detach(), true /* attachValues */));
                    // event incl. timeStamp, eventData
                    IODataProviderNamespace::Event ioEvent(event->getTimeStamp(), *ioEventData,
                            true /* attachValues */);
                    // send IO data provider event via callbacks
                    for (int i = 0; i < obj.callbacks.size(); i++) {
                        CallbackData& callbackData = *obj.callbacks[i];
                        if (callbackData.nodeId->equals(*ioEventTypeId)) {
                            callbackData.callback->valuesChanged(ioEvent); // SubscriberCallbackException
                        }
                    }
                    break;
                }
                default:
                {
                    // add message to queue
                    // TODO set expiration time for message
                    try {
                        obj.incomingQueue->put(*message, incomingMessageExpirationTime); //TimeoutException
                        messageSG.detach();
                    } catch (TimeoutException& e) {
                        std::ostringstream msg;
                        msg << "Received message of type "
                                << message->getMessageHeader().getMessageType()
                                << " with identifier " << message->getMessageHeader().getMessageId()
                                << " expired after " << incomingMessageExpirationTime << " sec.";
                        TimeoutException ex = ExceptionDef(TimeoutException, msg.str());
                        ex.setCause(&e);
                        // there is no way to inform the OPC UA server => log the exception
                        std::string st;
                        ex.getStackTrace(st);
                        obj.log->warn("Exception while receiving messages: %s", st.c_str());
                        throw ex;
                    }
                }
            }
        } catch (ClientSocketException& e) {
            // there is no way to inform the OPC UA server => log the exception            
            std::string st;
            e.getStackTrace(st);
            obj.log->error("Exception while processing incoming messages: %s", st.c_str());
            reopenClientSocket = true;
        } catch (TimeoutException& e) {
        } catch (Exception& e) {
            // there is no way to inform the OPC UA server => log the exception
            std::string st;
            e.getStackTrace(st);
            obj.log->error("Exception while processing incoming messages: %s", st.c_str());
        }
        MutexLock lock(*obj.mutex);
        isStopped = !obj.isListening;
    }
    obj.log->info("Stopping thread for receiving messages");

    return NULL;
}

void BinaryIODataProviderPrivate::sendMessage(
        Message & message) /* throws ClientSocketException */ {

    BinaryMessageSerializer s;
    unsigned char* requestBuffer = s.serialize(message);
    ScopeGuard<unsigned char> requestBufferSG(requestBuffer, true /*isArray*/);
    clientSocket->writeData(
            BinaryMessageSerializer::MESSAGE_HEADER_LENGTH
            + message.getMessageHeader().getMessageBodyLength(),
            requestBuffer);
}

void BinaryIODataProviderPrivate::loadConfig(
        const std::string & confFile) {
    // set default values
    connectTimeout = 120; // in sec.    
    sendReceiveTimeout = 30; // in sec.    
    maxReconnectDelay = 10; // in sec.
    maxIncomingMessageQueueSize = 100;
    incomingMessageExpirationTime = 60; // in sec.

    xmlInitParser();
    xmlDoc* doc = xmlReadFile(confFile.c_str(), NULL /*encoding*/, 0 /* options */);
    if (doc == NULL) {

        xmlCleanupParser();
        std::ostringstream msg;
        msg << "Cannot parse configuration file";
        throw ExceptionDef(IODataProviderNamespace::IODataProviderException, msg.str());
    }
    parseConfig(xmlDocGetRootElement(doc));
    xmlFreeDoc(doc);
    xmlCleanupParser();
}

void BinaryIODataProviderPrivate::parseConfig(
        const xmlNode * startNode) {
    xmlChar* remote = xmlCharStrdup("remote");
    xmlChar* host = xmlCharStrdup("host");
    xmlChar* port = xmlCharStrdup("port");
    xmlChar* connectTimeout = xmlCharStrdup("connectTimeout");
    xmlChar* sendReceiveTimeout = xmlCharStrdup("sendReceiveTimeout");
    xmlChar* maxReconnectDelay = xmlCharStrdup("maxReconnectDelay");

    xmlChar* messages = xmlCharStrdup("messages");
    xmlChar* incoming = xmlCharStrdup("incoming");
    xmlChar* maxIncomingMessageQueueSize = xmlCharStrdup("maxQueueSize");
    xmlChar* incomingMessageExpirationTime = xmlCharStrdup("expirationTime");
    xmlChar* ignoreResponseErrorStatus = xmlCharStrdup("ignoreResponseErrorStatus");

    for (const xmlNode* currNode = startNode; currNode != NULL; currNode =
            currNode->next) {
        if (currNode->type == XML_ELEMENT_NODE) {
            // remote
            if (xmlStrEqual(currNode->name, remote) != 0) {
                xmlAttr* props = currNode->properties;
                if (props != NULL) {
                    for (xmlAttr* prop = props; prop != NULL; prop =
                            prop->next) {
                        if (currNode->type == XML_ELEMENT_NODE) {
                            if (xmlStrEqual(prop->name, host) != 0) {
                                this->host = std::string(
                                        (char*) prop->children->content);
                            } else if (xmlStrEqual(prop->name, port) != 0) {
                                this->port = atoi(
                                        (char *) prop->children->content);
                            } else if (xmlStrEqual(prop->name, connectTimeout) != 0) {
                                this->connectTimeout = atoi(
                                        (char *) prop->children->content);
                            } else if (xmlStrEqual(prop->name, sendReceiveTimeout) != 0) {
                                this->sendReceiveTimeout = atoi(
                                        (char *) prop->children->content);
                            } else if (xmlStrEqual(prop->name, maxReconnectDelay) != 0) {
                                this->maxReconnectDelay = atoi(
                                        (char *) prop->children->content);
                            } else {
                                log->warn("Unknown configuration attribute found for element '%s' : % s",
                                        currNode->name, prop->name);
                            }
                        }
                    }
                }
                // incoming
            } else if (xmlStrEqual(currNode->name, incoming) != 0) {
                xmlAttr* props = currNode->properties;
                if (props != NULL) {
                    for (xmlAttr* prop = props; prop != NULL; prop =
                            prop->next) {
                        if (currNode->type == XML_ELEMENT_NODE) {
                            if (xmlStrEqual(prop->name, maxIncomingMessageQueueSize) != 0) {
                                this->maxIncomingMessageQueueSize = atoi(
                                        (char *) prop->children->content);
                            } else if (xmlStrEqual(prop->name, incomingMessageExpirationTime) != 0) {
                                this->incomingMessageExpirationTime = atoi(
                                        (char *) prop->children->content);
                            } else {
                                log->warn("Unknown configuration attribute found for element '%s' : % s",
                                        currNode->name, prop->name);
                            }
                        }
                    }
                }
            }
            parseConfig(currNode->children);
        }
    }
    // usage of libxml2: "Deallocating non-allocated memory"
    if (!unitTesting) {
        delete[] remote;
        delete[] port;
        delete[] host;
        delete[] connectTimeout;
        delete[] sendReceiveTimeout;
        delete[] maxReconnectDelay;

        delete[] messages;
        delete[] incoming;
        delete[] maxIncomingMessageQueueSize;
        delete[] incomingMessageExpirationTime;
    }
}
