#include "BinaryMessageDeserializer.h"
#include "BinaryMessageSerializer.h"
#include "MessageDeserializerException.h"
#include "dto/Array.h"
#include "dto/Call.h"
#include "dto/CallResponse.h"
#include "dto/Event.h"
#include "dto/Notification.h"
#include "dto/Read.h"
#include "dto/ReadResponse.h"
#include "dto/Status.h"
#include "dto/Struct.h"
#include "dto/Subscribe.h"
#include "dto/SubscribeResponse.h"
#include "dto/Unsubscribe.h"
#include "dto/UnsubscribeResponse.h"
#include "dto/Write.h"
#include "dto/WriteResponse.h"
#include <common/Exception.h>
#include <common/ScopeGuard.h>
#include <common/VectorScopeGuard.h>
#include <common/MapScopeGuard.h>
#include <common/logging/Logger.h>
#include <common/logging/LoggerFactory.h>
#include <netinet/in.h> // htons
#include <sstream> // std::ostringstream
#include <stddef.h> // size_t
#include <stdio.h> // asprintf
#include <stdlib.h> // free
#include <string.h> //memcpy
#include <vector>
#ifdef DEBUG
#include <CppUTest/MemoryLeakDetectorNewMacros.h>
#endif

using namespace CommonNamespace;

class BinaryMessageDeserializerPrivate {
    friend class BinaryMessageDeserializer;
private:
    Logger* log;
    std::string logMsg;
    bool isDebugEnabled;

    size_t charSize;
    size_t intSize;
    size_t longSize;
    size_t llongSize;
    size_t floatSize;
    size_t doubleSize;
    bool isBigEndian;

    Message* deserializeReadBody(const MessageHeader& messageHeader,
            unsigned char* buffer) /* throws MessageDeserializerException */;
    Message* deserializeReadResponseBody(const MessageHeader& messageHeader,
            unsigned char* buffer)/* throws MessageDeserializerException*/;
    Message* deserializeWriteBody(const MessageHeader& messageHeader,
            unsigned char* buffer) /* throws MessageDeserializerException */;
    Message* deserializeWriteResponseBody(const MessageHeader& messageHeader,
            unsigned char* buffer);
    Message* deserializeSubscribeBody(const MessageHeader& messageHeader,
            unsigned char* buffer) /* throws MessageDeserializerException */;
    Message* deserializeSubscribeResponseBody(
            const MessageHeader& messageHeader, unsigned char* buffer);
    Message* deserializeUnsubscribeBody(const MessageHeader& messageHeader,
            unsigned char* buffer) /* throws MessageDeserializerException */;
    Message* deserializeUnsubscribeResponseBody(
            const MessageHeader& messageHeader, unsigned char* buffer);
    Message* deserializeNotificationBody(const MessageHeader& messageHeader,
            unsigned char* buffer) /*throws MessageDeserializerException*/;
    Message* deserializeEventBody(const MessageHeader& messageHeader,
            unsigned char* buffer) /*throws MessageDeserializerException*/;
    Message* deserializeCallBody(const MessageHeader& messageHeader,
            unsigned char* buffer) /* throws MessageDeserializerException */;
    Message* deserializeCallResponseBody(const MessageHeader& messageHeader,
            unsigned char* buffer) /* throws MessageDeserializerException */;

    ParamId* deserializeParamId(const char* description,
            unsigned char** bufferPos) /* throws MessageDeserializerException */;
    ParamList* deserializeParamList(unsigned int noOfParams,
            unsigned char** bufferPos) /* throws MessageDeserializerException */;
    ParamMap* deserializeParamMap(unsigned int noOfParams,
            unsigned char** bufferPos) /*throws MessageDeserializerException*/;
    Variant* deserializeParamValue(int paramType, unsigned char** bufferPos) /* throws MessageDeserializerException */;
    Scalar* deserializeScalarValue(Scalar::Type scalarType,
            unsigned char** bufferPos) /* throws MessageDeserializerException */;
    Array* deserializeArrayValue(unsigned char** bufferPos)
    /* throws MessageDeserializerException */;
    Struct* deserializeStructValue(unsigned char** bufferPos)
    /* throws MessageDeserializerException */;

    bool read8bool(unsigned char** bufferPos);
    char read8char(unsigned char** bufferPos);
    signed char read8byte(unsigned char** bufferPos);
    int read8int(unsigned char** bufferPos);
    int read16int(unsigned char** bufferPos);
    unsigned int read16uint(unsigned char** bufferPos);
    long read32long(unsigned char** bufferPos);
    unsigned long read32ulong(unsigned char** bufferPos);
    float read32float(unsigned char** bufferPos);
    long long read64llong(unsigned char** bufferPos);
    double read64double(unsigned char** bufferPos);

    template<class TYPE>
    void read(unsigned char** bufferPos, size_t bufferSize,
            size_t destValueSize, TYPE* returnDestValue);
};

BinaryMessageDeserializer::BinaryMessageDeserializer() {
    d = new BinaryMessageDeserializerPrivate();
    d->log = LoggerFactory::getLogger("BinaryMessageDeserializer");
    d->charSize = sizeof (char);
    d->intSize = sizeof (int);
    d->longSize = sizeof (long);
    d->llongSize = sizeof (long long);
    d->floatSize = sizeof (float);
    d->doubleSize = sizeof (double);
    d->isBigEndian = 1 == htons(1);
}

BinaryMessageDeserializer::~BinaryMessageDeserializer() {
    delete d;
}

MessageHeader* BinaryMessageDeserializer::deserializeMessageHeader(
        unsigned char* buffer) {
    d->logMsg.clear();
    d->isDebugEnabled = d->log->isDebugEnabled();
    if (d->isDebugEnabled) {
        d->logMsg.append("MessageHeader[messageType=");
    }
    int messageType = d->read16int(&buffer);
    if (d->isDebugEnabled) {
        d->logMsg.append(",messageLength=");
    }
    int messageLength = d->read32ulong(&buffer);
    if (d->isDebugEnabled) {
        d->logMsg.append(",messageId=");
    }
    unsigned long messageId = d->read32ulong(&buffer);
    if (d->isDebugEnabled) {
        d->logMsg.append("]");
        d->log->debug("%s", d->logMsg.c_str());
        d->logMsg.clear();
    }
    MessageHeader* messageHeader = new MessageHeader(
            (MessageHeader::MessageType) messageType, messageId);
    messageHeader->setMessageBodyLength(
            messageLength - BinaryMessageSerializer::MESSAGE_HEADER_LENGTH);
    return messageHeader;
}

Message* BinaryMessageDeserializer::deserializeMessageBody(
        const MessageHeader& messageHeader,
        unsigned char* buffer) /* throws MessageDeserializerException */ {
    d->isDebugEnabled = d->log->isDebugEnabled();
    Message* ret;
    switch (messageHeader.getMessageType()) {
        case MessageHeader::READ:
            if (d->log->isInfoEnabled()) {
                d->log->info("ReadRequest id=%u,bodyLength=%u",
                        messageHeader.getMessageId(), messageHeader.getMessageBodyLength());
            }
            ret = d->deserializeReadBody(messageHeader, buffer);
            break;
        case MessageHeader::READ_RESPONSE:
            if (d->log->isInfoEnabled()) {
                d->log->info("ReadResponse id=%u,bodyLength=%u",
                        messageHeader.getMessageId(), messageHeader.getMessageBodyLength());
            }
            ret = d->deserializeReadResponseBody(messageHeader, buffer);
            break;
        case MessageHeader::WRITE:
            if (d->log->isInfoEnabled()) {
                d->log->info("WriteRequest id=%u,bodyLength=%u",
                        messageHeader.getMessageId(), messageHeader.getMessageBodyLength());
            }
            ret = d->deserializeWriteBody(messageHeader, buffer);
            break;
        case MessageHeader::WRITE_RESPONSE:
            if (d->log->isInfoEnabled()) {
                d->log->info("WriteResponse id=%u,bodyLength=%u",
                        messageHeader.getMessageId(), messageHeader.getMessageBodyLength());
            }
            ret = d->deserializeWriteResponseBody(messageHeader, buffer);
            break;
        case MessageHeader::SUBSCRIBE:
            if (d->log->isInfoEnabled()) {
                d->log->info("SubscribeRequest id=%u,bodyLength=%u",
                        messageHeader.getMessageId(), messageHeader.getMessageBodyLength());
            }
            ret = d->deserializeSubscribeBody(messageHeader, buffer);
            break;
        case MessageHeader::SUBSCRIBE_RESPONSE:
            if (d->log->isInfoEnabled()) {
                d->log->info("SubscribeResponse id=%u,bodyLength=%u",
                        messageHeader.getMessageId(), messageHeader.getMessageBodyLength());
            }
            ret = d->deserializeSubscribeResponseBody(messageHeader, buffer);
            break;
        case MessageHeader::UNSUBSCRIBE:
            if (d->log->isInfoEnabled()) {
                d->log->info("UnsubscribeRequest id=%u,bodyLength=%u",
                        messageHeader.getMessageId(), messageHeader.getMessageBodyLength());
            }
            ret = d->deserializeUnsubscribeBody(messageHeader, buffer);
            break;
        case MessageHeader::UNSUBSCRIBE_RESPONSE:
            if (d->log->isInfoEnabled()) {
                d->log->info("UnsubscribeResponse id=%u,bodyLength=%u",
                        messageHeader.getMessageId(), messageHeader.getMessageBodyLength());
            }
            ret = d->deserializeUnsubscribeResponseBody(messageHeader, buffer);
            break;
        case MessageHeader::NOTIFICATION:
            if (d->log->isInfoEnabled()) {
                d->log->info("Notification id=%u,bodyLength=%u",
                        messageHeader.getMessageId(), messageHeader.getMessageBodyLength());
            }
            ret = d->deserializeNotificationBody(messageHeader, buffer);
            break;
        case MessageHeader::EVENT:
            if (d->log->isInfoEnabled()) {
                d->log->info("Event id=%u,bodyLength=%u",
                        messageHeader.getMessageId(), messageHeader.getMessageBodyLength());
            }
            ret = d->deserializeEventBody(messageHeader, buffer);
            break;
        case MessageHeader::CALL:
            if (d->log->isInfoEnabled()) {
                d->log->info("CallRequest id=%u,bodyLength=%u",
                        messageHeader.getMessageId(), messageHeader.getMessageBodyLength());
            }
            ret = d->deserializeCallBody(messageHeader, buffer);
            break;
        case MessageHeader::CALL_RESPONSE:
            if (d->log->isInfoEnabled()) {
                d->log->info("CallResponse id=%u,bodyLength=%u",
                        messageHeader.getMessageId(), messageHeader.getMessageBodyLength());
            }
            ret = d->deserializeCallResponseBody(messageHeader, buffer);
            break;
        default:
            std::ostringstream msg;
            msg << "Unknown message type " << messageHeader.getMessageType();
            throw ExceptionDef(MessageDeserializerException, msg.str());
    }
    if (d->isDebugEnabled) {
        d->log->debug("%s", d->logMsg.c_str());
        d->logMsg.clear();
    }
    return ret;
}

Message* BinaryMessageDeserializerPrivate::deserializeReadBody(
        const MessageHeader& messageHeader,
        unsigned char* buffer) /* throws MessageDeserializerException */ {
    if (isDebugEnabled) {
        logMsg.append("ReadBody[");
    }
    ParamId* paramId = deserializeParamId("ParamId", &buffer); // MessageDeserializerException
    if (isDebugEnabled) {
        logMsg.append("]");
    }
    return new Read(messageHeader.getMessageId(), *paramId,
            true /* attachValues */);
}

Message* BinaryMessageDeserializerPrivate::deserializeReadResponseBody(
        const MessageHeader& messageHeader,
        unsigned char* buffer) /* throws MessageDeserializerException*/ {
    if (isDebugEnabled) {
        logMsg.append("ReadResponseBody[status=");
    }
    int status = read16int(&buffer);
    Status::Value statusValue = (Status::Value) status;
    ReadResponse* response = new ReadResponse(messageHeader.getMessageId(),
            statusValue, true /* attachValues */);
    ScopeGuard<ReadResponse> ret(response);
    if (statusValue == Status::SUCCESS) {
        if (isDebugEnabled) {
            logMsg.append(",");
        }
        ParamId* paramId = deserializeParamId("ParamId", &buffer); // MessageDeserializerException
        response->setParamId(paramId);
        if (isDebugEnabled) {
            logMsg.append(",type=");
        }
        int paramType = read16int(&buffer);
        if (isDebugEnabled) {
            logMsg.append(",value=");
        }
        Variant* paramValue = deserializeParamValue(paramType, &buffer); // MessageDeserializerException
        response->setParamValue(paramValue);
    }
    if (isDebugEnabled) {
        logMsg.append("]");
    }
    return ret.detach();
}

Message* BinaryMessageDeserializerPrivate::deserializeWriteBody(
        const MessageHeader& messageHeader,
        unsigned char* buffer) /* throws MessageDeserializerException */ {
    if (isDebugEnabled) {
        logMsg.append("WriteBody[");
    }
    ScopeGuard<ParamId> paramIdSG(deserializeParamId("ParamId", &buffer)); // MessageDeserializerException
    if (isDebugEnabled) {
        logMsg.append(",type=");
    }
    int paramType = read16int(&buffer);
    if (isDebugEnabled) {
        logMsg.append(",value=");
    }
    Variant* paramValue = deserializeParamValue(paramType, &buffer); // MessageDeserializerException
    if (isDebugEnabled) {
        logMsg.append("]");
    }
    return new Write(messageHeader.getMessageId(), *paramIdSG.detach(),
            *paramValue, true /* attachValues */);
}

Message* BinaryMessageDeserializerPrivate::deserializeWriteResponseBody(
        const MessageHeader& messageHeader, unsigned char* buffer) {
    if (isDebugEnabled) {
        logMsg.append("WriteResponseBody[status=");
    }
    int status = read16int(&buffer);
    if (isDebugEnabled) {
        logMsg.append("]");
    }
    return new WriteResponse(messageHeader.getMessageId(),
            (Status::Value) status);
}

Message* BinaryMessageDeserializerPrivate::deserializeSubscribeBody(
        const MessageHeader& messageHeader,
        unsigned char* buffer) /* throws MessageDeserializerException */ {
    if (isDebugEnabled) {
        logMsg.append("SubscribeBody[");
    }
    ParamId* paramId = deserializeParamId("ParamId", &buffer); // MessageDeserializerException
    if (isDebugEnabled) {
        logMsg.append("]");
    }
    return new Subscribe(messageHeader.getMessageId(), *paramId,
            true /* attachValues */);
}

Message* BinaryMessageDeserializerPrivate::deserializeSubscribeResponseBody(
        const MessageHeader& messageHeader, unsigned char* buffer) {
    if (isDebugEnabled) {
        logMsg.append("SubscribeResponseBody[status=");
    }
    int status = read16int(&buffer);
    if (isDebugEnabled) {
        logMsg.append("]");
    }
    return new SubscribeResponse(messageHeader.getMessageId(),
            (Status::Value) status);
}

Message* BinaryMessageDeserializerPrivate::deserializeUnsubscribeBody(
        const MessageHeader& messageHeader,
        unsigned char* buffer) /* throws MessageDeserializerException */ {
    if (isDebugEnabled) {
        logMsg.append("UnsubscribeBody[");
    }
    ParamId* paramId = deserializeParamId("ParamId", &buffer); // MessageDeserializerException
    if (isDebugEnabled) {
        logMsg.append("]");
    }
    return new Unsubscribe(messageHeader.getMessageId(), *paramId,
            true /* attachValues */);
}

Message* BinaryMessageDeserializerPrivate::deserializeUnsubscribeResponseBody(
        const MessageHeader& messageHeader, unsigned char* buffer) {
    if (isDebugEnabled) {
        logMsg.append("UnsubscribeResponseBody[status=");
    }
    int status = read16int(&buffer);
    if (isDebugEnabled) {
        logMsg.append("]");
    }
    return new UnsubscribeResponse(messageHeader.getMessageId(),
            (Status::Value) status);
}

Message* BinaryMessageDeserializerPrivate::deserializeNotificationBody(
        const MessageHeader& messageHeader,
        unsigned char* buffer) /* throws MessageDeserializerException */ {
    if (isDebugEnabled) {
        logMsg.append("NotificationBody[noOfParams=");
    }
    unsigned int noOfParams = read16uint(&buffer);
    if (isDebugEnabled) {
        logMsg.append(",");
    }
    ParamMap* paramMap = deserializeParamMap(noOfParams, &buffer); // MessageDeserializerException
    if (isDebugEnabled) {
        logMsg.append("]");
    }
    return new Notification(messageHeader.getMessageId(), *paramMap,
            true /* attachValues */);
}

Message* BinaryMessageDeserializerPrivate::deserializeEventBody(const MessageHeader& messageHeader,
        unsigned char* buffer) /* throws MessageDeserializerException */ {
    if (isDebugEnabled) {
        logMsg.append("EventBody[");
    }
    ScopeGuard<ParamId> eventIdSG(deserializeParamId("EventId", &buffer)); // MessageDeserializerException
    if (isDebugEnabled) {
        logMsg.append(",");
    }
    ScopeGuard<ParamId> paramIdSG(deserializeParamId("ParamId", &buffer)); // MessageDeserializerException
    if (isDebugEnabled) {
        logMsg.append(",timeStamp=");
    }
    long long timeStamp = read64llong(&buffer);
    if (isDebugEnabled) {
        logMsg.append(",severity=");
    }
    long severity = read32long(&buffer);
    // message is specified as string type, thus only length and chars are needed
    if (isDebugEnabled) {
        logMsg.append(",message=[length=");
    }
    unsigned int length = read16uint(&buffer);
    if (isDebugEnabled) {
        logMsg.append(",values=");
    }
    char msg[length + 1];
    for (unsigned int i = 0; i < length; i++) {
        if (isDebugEnabled && i > 0) {
            logMsg.append(" ");
        }
        char c = read8char(&buffer);
        msg[i] = c;
    }
    msg[length] = 0;
    ScopeGuard<std::string> messageSG(new std::string(msg));
    if (isDebugEnabled) {
        logMsg.append(" (").append(msg).append(")],noOfParams=");
    }
    unsigned int noOfParams = read16uint(&buffer);
    if (isDebugEnabled) {
        logMsg.append(",");
    }
    ParamMap* paramMap = deserializeParamMap(noOfParams, &buffer); // MessageDeserializerException
    if (isDebugEnabled) {
        logMsg.append("]");
    }
    return new Event(messageHeader.getMessageId(), *eventIdSG.detach(),
            *paramIdSG.detach(), timeStamp, severity, *messageSG.detach(), *paramMap,
            true /* attachValues */);
}

Message* BinaryMessageDeserializerPrivate::deserializeCallBody(
        const MessageHeader& messageHeader,
        unsigned char* buffer) /* throws MessageDeserializerException */ {
    if (isDebugEnabled) {
        logMsg.append("CallBody[");
    }
    ScopeGuard<ParamId> methodId(deserializeParamId("MethodId", &buffer)); // MessageDeserializerException
    if (isDebugEnabled) {
        logMsg.append(",");
    }
    ScopeGuard<ParamId> paramId(deserializeParamId("ParamId", &buffer)); // MessageDeserializerException
    if (isDebugEnabled) {
        logMsg.append(",noOfParams=");
    }
    unsigned int noOfParams = read16uint(&buffer);
    if (isDebugEnabled) {
        logMsg.append(",");
    }
    ParamList* paramList = deserializeParamList(noOfParams, &buffer); // MessageDeserializerException
    if (isDebugEnabled) {
        logMsg.append("]");
    }
    return new Call(messageHeader.getMessageId(), *methodId.detach(),
            *paramId.detach(), *paramList, true /* attachValues */);
}

Message* BinaryMessageDeserializerPrivate::deserializeCallResponseBody(
        const MessageHeader& messageHeader,
        unsigned char* buffer) /* throws MessageDeserializerException */ {
    if (isDebugEnabled) {
        logMsg.append("CallResponseBody[status=");
    }
    int status = read16int(&buffer);
    CallResponse* ret = new CallResponse(messageHeader.getMessageId(),
            (Status::Value) status, true /* attachValues */);
    switch (status) {
        case Status::SUCCESS:
        case Status::APPLICATION_ERROR:
        {
            if (isDebugEnabled) {
                logMsg.append(",");
            }
            ScopeGuard<ParamId> methodId(deserializeParamId("MethodId", &buffer)); // MessageDeserializerException
            if (isDebugEnabled) {
                logMsg.append(",");
            }
            ScopeGuard<ParamId> paramId(deserializeParamId("ParamId", &buffer)); // MessageDeserializerException
            if (isDebugEnabled) {
                logMsg.append(",noOfParams=");
            }
            unsigned int noOfParams = read16uint(&buffer);
            if (isDebugEnabled) {
                logMsg.append(",");
            }
            ParamList* paramList = deserializeParamList(noOfParams, &buffer); // MessageDeserializerException        
            ret->setMethodId(methodId.detach());
            ret->setParamId(paramId.detach());
            ret->setParamList(paramList);
            break;
        }
    }
    if (isDebugEnabled) {
        logMsg.append("]");
    }
    return ret;
}

ParamId* BinaryMessageDeserializerPrivate::deserializeParamId(const char* description,
        unsigned char** bufferPos) /*throws MessageDeserializerException*/ {
    if (isDebugEnabled) {
        logMsg.append(description).append("[namespaceIndex=");
    }
    short namespaceIndex = (short)read16int(bufferPos);
    if (isDebugEnabled) {
        logMsg.append(",type=");
    }
    int paramIdType = read8int(bufferPos);
    ParamId* paramId;
    switch (paramIdType) {
        case ParamId::NUMERIC:
            if (isDebugEnabled) {
                logMsg.append(",value=");
            }
            paramId = new ParamId(namespaceIndex, read32ulong(bufferPos));
            break;
        case ParamId::STRING:
        {
            if (isDebugEnabled) {
                logMsg.append(",length=");
            }
            unsigned int len = read16uint(bufferPos);
            if (isDebugEnabled) {
                logMsg.append(",values=");
            }
            char str[len + 1];
            for (unsigned int i = 0; i < len; i++) {
                if (isDebugEnabled && i > 0) {
                    logMsg.append(" ");
                }
                str[i] = read8char(bufferPos);
            }
            str[len] = 0;
            if (isDebugEnabled) {
                logMsg.append(" (").append(str).append(")");
            }
            paramId = new ParamId(namespaceIndex, *new std::string(str), true /* attachValues */);
            break;
        }
        default:
            std::ostringstream msg;
            msg << "Unknown paramId type " << paramIdType;
            throw ExceptionDef(MessageDeserializerException, msg.str());
            break;
    }
    if (isDebugEnabled) {
        logMsg.append("]");
    }
    return paramId;

}

ParamList* BinaryMessageDeserializerPrivate::deserializeParamList(
        unsigned int noOfParams,
        unsigned char** bufferPos)/* throws MessageDeserializerException */ {
    if (isDebugEnabled) {
        logMsg.append("ParamList[");
    }
    std::vector<const Variant*>* elements = new std::vector<const Variant*>();
    VectorScopeGuard<const Variant> elementsSG(elements);
    for (unsigned int i = 0; i < noOfParams; i++) {
        if (isDebugEnabled) {
            if (i > 0) {
                logMsg.append(" ");
            }
            logMsg.append("type=");
        }
        int paramType = read16int(bufferPos);
        if (isDebugEnabled) {
            logMsg.append(",value=");
        }
        elements->push_back(deserializeParamValue(paramType, bufferPos)); // MessageDeserializerException
    }
    if (isDebugEnabled) {
        logMsg.append("]");
    }
    return new ParamList(*elementsSG.detach(), true /* attachValues */);
}

ParamMap* BinaryMessageDeserializerPrivate::deserializeParamMap(
        unsigned int noOfParams,
        unsigned char** bufferPos)/* throws MessageDeserializerException */ {
    if (isDebugEnabled) {
        logMsg.append("ParamMap[");
    }
    std::map<const ParamId*, const Variant*>* elements = new std::map<
            const ParamId*, const Variant*>();
    try {
        for (unsigned int i = 0; i < noOfParams; i++) {
            if (isDebugEnabled && i > 0) {
                logMsg.append(",");
            }
            ScopeGuard<ParamId> paramId(
                    deserializeParamId("ParamId", bufferPos)); // MessageDeserializerException
            if (isDebugEnabled) {
                logMsg.append(",type=");
            }
            int paramType = read16int(bufferPos);
            if (isDebugEnabled) {
                logMsg.append(",value=");
            }
            Variant* paramValue = deserializeParamValue(paramType, bufferPos); // MessageDeserializerException
            (*elements)[paramId.detach()] = paramValue;
        }
        if (isDebugEnabled) {
            logMsg.append("]");
        }
    } catch (Exception& e) {
        // delete existing elements
        ParamMap pm(*elements, true /*attachValues*/);
    }
    return new ParamMap(*elements, true /* attachValues */);
}

Variant* BinaryMessageDeserializerPrivate::deserializeParamValue(
        int paramType, unsigned char** bufferPos)/* throws MessageDeserializerException */ {
    switch (paramType) {
        case Scalar::BOOLEAN: //0
        case Scalar::CHAR:
        case Scalar::BYTE:
        case Scalar::SHORT:
        case Scalar::INT:
        case Scalar::LONG:
        case Scalar::FLOAT:
        case Scalar::DOUBLE: // 7
            return deserializeScalarValue((Scalar::Type) paramType, bufferPos); // MessageDeserializerException
        case Variant::ARRAY: // 8
            return deserializeArrayValue(bufferPos); // MessageDeserializerException
        case Variant::STRUCT: // 9
            return deserializeStructValue(bufferPos); // MessageDeserializerException
        default:
            std::ostringstream msg;
            msg << "Unknown paramType " << paramType;
            throw ExceptionDef(MessageDeserializerException, msg.str());
            break;
    }
}

Scalar* BinaryMessageDeserializerPrivate::deserializeScalarValue(
        Scalar::Type scalarType,
        unsigned char** bufferPos) /* throws MessageDeserializerException */ {
    Scalar* paramValue = new Scalar();
    ScopeGuard<Scalar> ret(paramValue);
    switch (scalarType) {
        case Scalar::BOOLEAN:
            paramValue->setBoolean(read8bool(bufferPos));
            break;
        case Scalar::CHAR:
            paramValue->setChar(read8char(bufferPos));
            break;
        case Scalar::BYTE:
            paramValue->setByte(read8byte(bufferPos));
            break;
        case Scalar::SHORT:
            paramValue->setShort(read16int(bufferPos));
            break;
        case Scalar::INT:
            paramValue->setInt(read32long(bufferPos));
            break;
        case Scalar::LONG:
            paramValue->setLong(read64llong(bufferPos));
            break;
        case Scalar::FLOAT:
            paramValue->setFloat(read32float(bufferPos));
            break;
        case Scalar::DOUBLE:
            paramValue->setDouble(read64double(bufferPos));
            break;
        default:
            std::ostringstream msg;
            msg << "Unknown scalar type " << scalarType;
            throw ExceptionDef(MessageDeserializerException, msg.str());
    }
    return ret.detach();
}

Array* BinaryMessageDeserializerPrivate::deserializeArrayValue(
        unsigned char** bufferPos) /* throws MessageDeserializerException */ {
    if (isDebugEnabled) {
        logMsg.append("Array[type=");
    }
    int arrayType = read16int(bufferPos);
    std::vector<const Variant*>* elements = new std::vector<const Variant*>();
    ScopeGuard<Array> arraySG(new Array(arrayType, *elements, true /* attachValues */));
    if (isDebugEnabled) {
        logMsg.append(",length=");
    }
    unsigned int arrayLength = read16uint(bufferPos);
    for (unsigned int i = 0; i < arrayLength; i++) {
        if (isDebugEnabled) {
            logMsg.append(",value=");
        }
        Variant* arrayValue = deserializeParamValue(arrayType, bufferPos); // MessageDeserializerException
        elements->push_back(arrayValue);
    }
    if (isDebugEnabled) {
        logMsg.append("]");
    }
    return arraySG.detach();
}

Struct* BinaryMessageDeserializerPrivate::deserializeStructValue(
        unsigned char** bufferPos) /* throws MessageDeserializerException */ {
    if (isDebugEnabled) {
        logMsg.append("Struct[");
    }
    ParamId* structId = deserializeParamId("StructId", bufferPos); // MessageDeserializerException
    if (isDebugEnabled) {
        logMsg.append(",fieldCount=");
    }
    unsigned int fieldCount = read16uint(bufferPos);
    if (isDebugEnabled) {
        logMsg.append(",");
    }
    std::map<std::string, const Variant*>* fields =
            new std::map<std::string, const Variant*>();
    ScopeGuard<Struct> structSG(new Struct(*structId, *fields, true /* attachValues*/));
    for (size_t i = 0; i < fieldCount; i++) {
        if (isDebugEnabled) {
            if (i > 0) {
                logMsg.append(" ");
            }
            // field name is specified as string type, thus only length and chars are needed
            logMsg.append("fieldName=[length=");
        }
        unsigned int arrayLength = read16uint(bufferPos);
        if (isDebugEnabled) {
            logMsg.append(",values=");
        }
        char fieldName[arrayLength + 1];
        for (size_t j = 0; j < arrayLength; j++) {
            if (isDebugEnabled && j > 0) {
                logMsg.append(" ");
            }
            fieldName[j] = read8char(bufferPos);
        }
        fieldName[arrayLength] = 0;
        if (isDebugEnabled) {
            logMsg.append(" (").append(fieldName).append(")],fieldType=");
        }
        int fieldType = read16int(bufferPos);
        if (isDebugEnabled) {
            logMsg.append(",fieldValue=");
        }
        Variant* fieldValue = deserializeParamValue(fieldType, bufferPos); // MessageDeserializerException
        (*fields)[std::string(fieldName)] = fieldValue;
    }
    if (isDebugEnabled) {
        logMsg.append("]");
    }
    return structSG.detach();
}

bool BinaryMessageDeserializerPrivate::read8bool(unsigned char** bufferPos) {
    bool ret = **bufferPos != 0;
    if (isDebugEnabled) {
        char* s;
        asprintf(&s, "0x_%x->%s/0x%x", **bufferPos, ret ? "true" : "false", **bufferPos);
        logMsg.append(s);
        free(s);
    }
    *bufferPos += charSize;
    return ret;
}

char BinaryMessageDeserializerPrivate::read8char(unsigned char** bufferPos) {
    char ret = 0;
    read(bufferPos, 1, charSize, &ret);
    if (isDebugEnabled) {
        char* s;
        asprintf(&s, "->%d/0x%x", ret, ret);
        logMsg.append(s);
        free(s);
    }
    return ret;
}

signed char BinaryMessageDeserializerPrivate::read8byte(unsigned char** bufferPos) {
    signed char ret = 0;
    read(bufferPos, 1, charSize, &ret);
    if (isDebugEnabled) {
        char* s;
        asprintf(&s, "->%d/0x%x", ret, ret);
        logMsg.append(s);
        free(s);
    }
    return ret;
}

int BinaryMessageDeserializerPrivate::read8int(unsigned char** bufferPos) {
    int ret = 0;
    read(bufferPos, 1, intSize, &ret);
    if (isDebugEnabled) {
        char* s;
        asprintf(&s, "->%d/0x%x", ret, ret);
        logMsg.append(s);
        free(s);
    }
    return ret;
}

int BinaryMessageDeserializerPrivate::read16int(unsigned char** bufferPos) {
    int ret = 0;
    read(bufferPos, 2, intSize, &ret);
    if (isDebugEnabled) {
        char* s;
        asprintf(&s, "->%d/0x%x", ret, ret);
        logMsg.append(s);
        free(s);
    }
    return ret;
}

unsigned int BinaryMessageDeserializerPrivate::read16uint(
        unsigned char** bufferPos) {
    unsigned int ret = 0;
    read(bufferPos, 2, intSize, &ret);
    if (isDebugEnabled) {
        char* s;
        asprintf(&s, "->%u/0x%x", ret, ret);
        logMsg.append(s);
        free(s);
    }
    return ret;
}

long BinaryMessageDeserializerPrivate::read32long(unsigned char** bufferPos) {
    long ret = 0;
    read(bufferPos, 4, longSize, &ret);
    if (isDebugEnabled) {
        char* s;
        asprintf(&s, "->%ld/0x%lx", ret, ret);
        logMsg.append(s);
        free(s);
    }
    return ret;
}

unsigned long BinaryMessageDeserializerPrivate::read32ulong(
        unsigned char** bufferPos) {
    unsigned long ret = 0;
    read(bufferPos, 4, longSize, &ret);
    if (isDebugEnabled) {
        char* s;
        asprintf(&s, "->%lu/0x%lx", ret, ret);
        logMsg.append(s);
        free(s);
    }
    return ret;
}

float BinaryMessageDeserializerPrivate::read32float(unsigned char** bufferPos) {
    unsigned long ulongValue = 0;
    read(bufferPos, 4, longSize, &ulongValue);
    float value = 0;
    if (isBigEndian) {
        memcpy(&value, &ulongValue + (longSize - floatSize), floatSize);
    } else {
        memcpy(&value, &ulongValue, floatSize);
    }
    if (isDebugEnabled) {
        char* s;
        asprintf(&s, "->%f/0x%lx", value, ulongValue);
        logMsg.append(s);
        free(s);
    }
    return value;
}

long long BinaryMessageDeserializerPrivate::read64llong(
        unsigned char** bufferPos) {
    long long ret = 0;
    read(bufferPos, 8, llongSize, &ret);
    if (isDebugEnabled) {
        char* s;
        asprintf(&s, "->%lld/0x%llx", ret, ret);
        logMsg.append(s);
        free(s);
    }
    return ret;
}

double BinaryMessageDeserializerPrivate::read64double(
        unsigned char** bufferPos) {
    unsigned long long ullongValue = 0;
    read(bufferPos, 8, llongSize, &ullongValue);
    double value = 0;
    if (isBigEndian) {
        memcpy(&value, &ullongValue + (llongSize - doubleSize), doubleSize);
    } else {
        memcpy(&value, &ullongValue, doubleSize);
    }
    if (isDebugEnabled) {
        char* s;
        asprintf(&s, "->%f/0x%llx", value, ullongValue);
        logMsg.append(s);
        free(s);
    }
    return value;
}

template<class TYPE>
void BinaryMessageDeserializerPrivate::read(unsigned char** bufferPos,
        size_t bufferSize, size_t destValueSize, TYPE* returnDestValue) {
    if (isDebugEnabled) {
        logMsg.append("0x");
        for (unsigned char* i = *bufferPos; i < *bufferPos + bufferSize * charSize;
                i += charSize) {
            char* s;
            asprintf(&s, "_%x", *i);
            logMsg.append(s);
            free(s);
        }
    }
    int cs;
    if (isBigEndian) {
        // append bytes in reverse order to destination value
        *bufferPos += bufferSize - charSize;
        cs = -charSize;
    } else {
        cs = charSize;
    }
    for (int i = bufferSize - charSize; i >= 0; i -= charSize) {
        // append char to destination value
        *returnDestValue |= ((TYPE) **bufferPos) << (i * 8);
        *bufferPos += cs;
    }
    if (isBigEndian) {
        *bufferPos += bufferSize + 1;
    }

    // bufferSize < destValueSize:
    // bufferSize 2, destValueSize 4, charSize 2:
    //   00 00 00 00
    //         12 34 << 0
    // ->00 00 12 34
    // bufferSize 4, destValueSize 6, charSize 2:
    //   00 00 00 00 00 00
    //               12 34 << 2
    //   00 00 12 34 00 00
    //               56 78 << 0
    // ->00 00 12 34 56 78

    // bufferSize == destValueSize:
    // bufferSize 2, destValueSize 2, charSize 2:
    //   00 00
    //   12 34 << 0
    // = 12 34
    // bufferSize 4, destValueSize 4, charSize 2:
    //   00 00 00 00
    //         12 34 << 2
    //   12 34 00 00
    //         56 78 << 0
    // ->12 34 56 78

    // bufferSize > destValueSize:
    // bufferSize 4, destValueSize 2, charSize 2:
    //   00 00
    //   12 34 << 2
    //   00 00
    //   56 78 << 0
    // = 56 78
    // bufferSize 6, destValueSize 4, charSize 2:
    //   00 00 00 00
    //         12 34 << 4
    //   00 00 00 00
    //         56 78 << 2
    //   56 78 00 00
    //         9A BC << 0
    // ->56 78 9A BC
    // bufferSize 8, destValueSize 4, charSize 2:
    //   00 00 00 00
    //         12 34 << 6
    //   00 00 00 00
    //         56 78 << 4
    //   00 00 00 00
    //         9A BC << 2
    //   9A BC 00 00
    //         DE F0 << 0
    // ->9A BC DE F0
}
