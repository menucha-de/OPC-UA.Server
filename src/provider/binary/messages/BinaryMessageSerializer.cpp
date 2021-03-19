#include "BinaryMessageSerializer.h"
#include "MessageSerializerException.h"
#include "dto/Array.h"
#include "dto/Call.h"
#include "dto/CallResponse.h"
#include "dto/Event.h"
#include "dto/Notification.h"
#include "dto/ParamIdMessage.h"
#include "dto/ParamList.h"
#include "dto/ParamMap.h"
#include "dto/Read.h"
#include "dto/ReadResponse.h"
#include "dto/StatusMessage.h"
#include "dto/Scalar.h"
#include "dto/Struct.h"
#include "dto/Subscribe.h"
#include "dto/SubscribeResponse.h"
#include "dto/Unsubscribe.h"
#include "dto/UnsubscribeResponse.h"
#include "dto/Variant.h"
#include "dto/Write.h"
#include "dto/WriteResponse.h"
#include <common/logging/Logger.h>
#include <common/logging/LoggerFactory.h>
#include <netinet/in.h> // htons
#include <sstream> // std::ostringstream
#include <stddef.h> // size_t
#include <stdio.h> // asprintf
#include <stdlib.h> // free
#include <string.h> // memcpy
#ifdef DEBUG
#include <CppUTest/MemoryLeakDetectorNewMacros.h>
#endif

using namespace CommonNamespace;

class BinaryMessageSerializerPrivate {
    friend class BinaryMessageSerializer;
private:
    Logger* log;
    std::string logMsg;
    bool isDebugEnabled;

    size_t charSize;
    char charMask;
    size_t longSize;
    size_t llongSize;
    size_t floatSize;
    size_t doubleSize;
    bool isBigEndian;

    unsigned long getMessagePartLength(Message& message);
    unsigned long getMessagePartLength(const Read& message);
    void serialize(const Read& message, unsigned char* buffer);
    unsigned long getMessagePartLength(ReadResponse& message);
    void serialize(ReadResponse& message, unsigned char* buffer);
    unsigned long getMessagePartLength(Write& message);
    void serialize(Write& message, unsigned char* buffer);
    unsigned long getMessagePartLength(WriteResponse& message);
    void serialize(WriteResponse& message, unsigned char* buffer);
    unsigned long getMessagePartLength(Subscribe& message);
    void serialize(Subscribe& message, unsigned char* buffer);
    unsigned long getMessagePartLength(SubscribeResponse& message);
    void serialize(SubscribeResponse& message, unsigned char* buffer);
    unsigned long getMessagePartLength(Unsubscribe& message);
    void serialize(Unsubscribe& message, unsigned char* buffer);
    unsigned long getMessagePartLength(UnsubscribeResponse& message);
    void serialize(UnsubscribeResponse& message, unsigned char* buffer);
    unsigned long getMessagePartLength(Notification& message);
    void serialize(Notification& message, unsigned char* buffer);
    unsigned long getMessagePartLength(Event& message);
    void serialize(Event& message, unsigned char* buffer);
    unsigned long getMessagePartLength(Call& message);
    void serialize(Call& message, unsigned char* buffer);
    unsigned long getMessagePartLength(CallResponse& message);
    void serialize(CallResponse& message, unsigned char* buffer);

    void serialize(const MessageHeader& messageHeader,
            unsigned char** bufferPos);
    unsigned long getMessagePartLength(ParamIdMessage& message);
    void serialize(ParamIdMessage& message, const char* description,
            unsigned char** bufferPos);
    unsigned long getMessagePartLength(StatusMessage& message);
    void serialize(StatusMessage& message, const char* description,
            unsigned char** bufferPos);
    unsigned long getMessagePartLength(const ParamId& paramId);
    void serialize(const ParamId& paramId, const char* description,
            unsigned char** bufferPos);
    unsigned long getMessagePartLength(const ParamList& paramList);
    void serialize(const ParamList& paramList, unsigned char** bufferPos);
    unsigned long getMessagePartLength(const ParamMap& paramMap);
    void serialize(const ParamMap& paramList, unsigned char** bufferPos);
    void serializeParamTypeValue(const Variant& paramValue, unsigned char** bufferPos)
    /* throws MessageSerializerException */;
    unsigned long getMessagePartLengthParamValue(const Variant& param);
    void serializeParamValue(const Variant& paramValue, unsigned char** bufferPos)
    /* throws MessageSerializerException */;
    unsigned long getMessagePartLengthScalarValue(const Scalar& value)
    /* throws MessageSerializerException */;
    void serializeScalarValue(const Scalar& value, unsigned char** bufferPos)
    /* throws MessageSerializerException */;
    unsigned long getMessagePartLengthArrayValue(const Array& value)
    /* throws MessageSerializerException */;
    void serializeArrayValue(const Array& value, unsigned char** bufferPos)
    /* throws MessageSerializerException */;
    unsigned long getMessagePartLengthStructValue(const Struct& value)
    /* throws MessageSerializerException */;
    void serializeStructValue(const Struct& value, unsigned char** bufferPos)
    /* throws MessageSerializerException */;

    void write8(bool value, unsigned char** bufferPos);
    void write8(char value, unsigned char** bufferPos);
    void write8(int value, unsigned char** bufferPos);
    void write16(int value, unsigned char** bufferPos);
    void write16(unsigned int value, unsigned char** bufferPos);
    void write32(long value, unsigned char** bufferPos);
    void write32(unsigned long value, unsigned char** bufferPos);
    void write32(float value, unsigned char** bufferPos);
    void write64(long long value, unsigned char** bufferPos);
    void write64(double value, unsigned char** bufferPos);

    template<class TYPE>
    void write(TYPE value, unsigned char** bufferPos, size_t bufferSize);
};

BinaryMessageSerializer::BinaryMessageSerializer() {
    d = new BinaryMessageSerializerPrivate();
    d->log = LoggerFactory::getLogger("BinaryMessageSerializer");
    d->charSize = sizeof (char);
    d->charMask = 0;
    d->longSize = sizeof (long);
    d->llongSize = sizeof (long long);
    d->floatSize = sizeof (float);
    d->doubleSize = sizeof (double);
    for (size_t i = 0; i < d->charSize; i++) {
        d->charMask |= 0xFF << (i * 8);
    }
    d->isBigEndian = 1234 == htons(1234);
}

BinaryMessageSerializer::~BinaryMessageSerializer() {
    delete d;
}

unsigned long BinaryMessageSerializerPrivate::getMessagePartLength(
        Message& message) /* throws MessageSerializerException */ {
    switch (message.getMessageHeader().getMessageType()) {
        case MessageHeader::READ:
            return getMessagePartLength((Read&) message);
        case MessageHeader::READ_RESPONSE:
            return getMessagePartLength((ReadResponse&) message);
        case MessageHeader::WRITE:
            return getMessagePartLength((Write&) message);
        case MessageHeader::WRITE_RESPONSE:
            return getMessagePartLength((WriteResponse&) message);
        case MessageHeader::SUBSCRIBE:
            return getMessagePartLength((Subscribe&) message);
        case MessageHeader::SUBSCRIBE_RESPONSE:
            return getMessagePartLength((SubscribeResponse&) message);
        case MessageHeader::UNSUBSCRIBE:
            return getMessagePartLength((Unsubscribe&) message);
        case MessageHeader::UNSUBSCRIBE_RESPONSE:
            return getMessagePartLength((UnsubscribeResponse&) message);
        case MessageHeader::NOTIFICATION:
            return getMessagePartLength((Notification&) message);
        case MessageHeader::EVENT:
            return getMessagePartLength((Event&) message);
        case MessageHeader::CALL:
            return getMessagePartLength((Call&) message);
        case MessageHeader::CALL_RESPONSE:
            return getMessagePartLength((CallResponse&) message);
        default:
            std::ostringstream msg;
            msg << "Unknown message type "
                    << message.getMessageHeader().getMessageType();
            throw ExceptionDef(MessageSerializerException, msg.str());
    }
    return 0;
}

unsigned char* BinaryMessageSerializer::serialize(
        Message& message) /* throws MessageSerializerException */ {
    d->isDebugEnabled = d->log->isDebugEnabled();
    // update message body length
    MessageHeader& messageHeader = message.getMessageHeader();
    unsigned long int messageLength = d->getMessagePartLength(message);
    messageHeader.setMessageBodyLength(messageLength - MESSAGE_HEADER_LENGTH);
    // serialize message
    unsigned char* buffer = new unsigned char[messageLength];
    switch (messageHeader.getMessageType()) {
        case MessageHeader::READ:
            if (d->log->isInfoEnabled()) {
                d->log->info("ReadRequest id=%u,bodyLength=%u",
                        messageHeader.getMessageId(), messageHeader.getMessageBodyLength());
            }
            d->serialize((Read&) message, buffer);
            break;
        case MessageHeader::READ_RESPONSE:
            if (d->log->isInfoEnabled()) {
                d->log->info("ReadResponse id=%u,bodyLength=%u",
                        messageHeader.getMessageId(), messageHeader.getMessageBodyLength());
            }
            d->serialize((ReadResponse&) message, buffer);
            break;
        case MessageHeader::WRITE:
            if (d->log->isInfoEnabled()) {
                d->log->info("WriteRequest id=%u,bodyLength=%u",
                        messageHeader.getMessageId(), messageHeader.getMessageBodyLength());
            }
            d->serialize((Write&) message, buffer); /* throws MessageSerializerException */
            break;
        case MessageHeader::WRITE_RESPONSE:
            if (d->log->isInfoEnabled()) {
                d->log->info("WriteResponse id=%u,bodyLength=%u",
                        messageHeader.getMessageId(), messageHeader.getMessageBodyLength());
            }
            d->serialize((WriteResponse&) message, buffer);
            break;
        case MessageHeader::SUBSCRIBE:
            if (d->log->isInfoEnabled()) {
                d->log->info("SubscribeRequest id=%u,bodyLength=%u",
                        messageHeader.getMessageId(), messageHeader.getMessageBodyLength());
            }
            d->serialize((Subscribe&) message, buffer);
            break;
        case MessageHeader::SUBSCRIBE_RESPONSE:
            if (d->log->isInfoEnabled()) {
                d->log->info("SubscribeResponse id=%u,bodyLength=%u",
                        messageHeader.getMessageId(), messageHeader.getMessageBodyLength());
            }
            d->serialize((SubscribeResponse&) message, buffer);
            break;
        case MessageHeader::UNSUBSCRIBE:
            if (d->log->isInfoEnabled()) {
                d->log->info("UnsubscribeRequest id=%u,bodyLength=%u",
                        messageHeader.getMessageId(), messageHeader.getMessageBodyLength());
            }
            d->serialize((Unsubscribe&) message, buffer);
            break;
        case MessageHeader::UNSUBSCRIBE_RESPONSE:
            if (d->log->isInfoEnabled()) {
                d->log->info("UnsubscribeResponse id=%u,bodyLength=%u",
                        messageHeader.getMessageId(), messageHeader.getMessageBodyLength());
            }
            d->serialize((UnsubscribeResponse&) message, buffer);
            break;
        case MessageHeader::NOTIFICATION:
            if (d->log->isInfoEnabled()) {
                d->log->info("Notification id=%u,bodyLength=%u",
                        messageHeader.getMessageId(), messageHeader.getMessageBodyLength());
            }
            d->serialize((Notification&) message, buffer);
            break;
        case MessageHeader::EVENT:
            if (d->log->isInfoEnabled()) {
                d->log->info("Event id=%u,bodyLength=%u",
                        messageHeader.getMessageId(), messageHeader.getMessageBodyLength());
            }
            d->serialize((Event&) message, buffer);
            break;
        case MessageHeader::CALL:
            if (d->log->isInfoEnabled()) {
                d->log->info("CallRequest id=%u,bodyLength=%u",
                        messageHeader.getMessageId(), messageHeader.getMessageBodyLength());
            }
            d->serialize((Call&) message, buffer);
            break;
        case MessageHeader::CALL_RESPONSE:
            if (d->log->isInfoEnabled()) {
                d->log->info("CallResponse id=%u,bodyLength=%u",
                        messageHeader.getMessageId(), messageHeader.getMessageBodyLength());
            }
            d->serialize((CallResponse&) message, buffer);
            break;
        default:
            std::ostringstream msg;
            msg << "Unknown message type " << messageHeader.getMessageType();
            throw ExceptionDef(MessageSerializerException, msg.str());
    }
    if (d->isDebugEnabled) {
        d->log->debug("%s", d->logMsg.c_str());
        d->logMsg.clear();
    }
    return buffer;
}

unsigned long BinaryMessageSerializerPrivate::getMessagePartLength(
        const Read& message) /* throws MessageSerializerException */ {
    return getMessagePartLength((ParamIdMessage&) message);
}

void BinaryMessageSerializerPrivate::serialize(const Read& message,
        unsigned char* buffer) /* throws MessageSerializerException */ {
    serialize((ParamIdMessage&) message, "Read", &buffer);
}

unsigned long BinaryMessageSerializerPrivate::getMessagePartLength(
        ReadResponse& message) /* throws MessageSerializerException */ {
    unsigned long len = BinaryMessageSerializer::MESSAGE_HEADER_LENGTH + 2; /* status (2 bytes) */
    if (message.getStatus() == Status::SUCCESS) {
        len += 2 /* paramType (2 bytes) */
                + getMessagePartLength(*message.getParamId())
                + getMessagePartLengthParamValue(*message.getParamValue());
    }
    return len;
}

void BinaryMessageSerializerPrivate::serialize(ReadResponse& message,
        unsigned char* buffer) /* throws MessageSerializerException */ {
    ReadResponse& msg = (ReadResponse&) message;
    if (isDebugEnabled) {
        logMsg.append("ReadResponse[");
    }
    serialize(msg.getMessageHeader(), &buffer);
    if (isDebugEnabled) {
        logMsg.append(",status=");
    }
    write16(msg.getStatus(), &buffer);
    if (msg.getStatus() == Status::SUCCESS) {
        if (isDebugEnabled) {
            logMsg.append(",");
        }
        serialize(*msg.getParamId(), "ParamId", &buffer);
        if (isDebugEnabled) {
            logMsg.append(",type=");
        }
        serializeParamTypeValue(*msg.getParamValue(), &buffer); // MessageSerializerException
        if (isDebugEnabled) {
            logMsg.append(",value=");
        }
        serializeParamValue(*msg.getParamValue(), &buffer);
    }
    if (isDebugEnabled) {
        logMsg.append("]");
    }
}

unsigned long BinaryMessageSerializerPrivate::getMessagePartLength(
        Write& message) /* throws MessageSerializerException */ {
    return BinaryMessageSerializer::MESSAGE_HEADER_LENGTH
            + 2 /* paramType (2 bytes) */
            + getMessagePartLength(message.getParamId())
            + getMessagePartLengthParamValue(message.getParamValue());
}

void BinaryMessageSerializerPrivate::serialize(Write& message,
        unsigned char* buffer)/* throws MessageSerializerException */ {
    Write& msg = (Write&) message;
    if (isDebugEnabled) {
        logMsg.append("Write[");
    }
    serialize(msg.getMessageHeader(), &buffer);
    if (isDebugEnabled) {
        logMsg.append(",");
    }
    serialize(msg.getParamId(), "ParamId", &buffer);
    if (isDebugEnabled) {
        logMsg.append(",type=");
    }
    serializeParamTypeValue(msg.getParamValue(), &buffer); // MessageSerializerException
    if (isDebugEnabled) {
        logMsg.append(",value=");
    }
    serializeParamValue(msg.getParamValue(), &buffer); // MessageSerializerException
    if (isDebugEnabled) {
        logMsg.append("]");
    }
}

unsigned long BinaryMessageSerializerPrivate::getMessagePartLength(
        WriteResponse& message) {
    return getMessagePartLength((StatusMessage&) message);
}

void BinaryMessageSerializerPrivate::serialize(WriteResponse& message,
        unsigned char* buffer) {
    serialize((StatusMessage&) message, "WriteResponse", &buffer);
}

unsigned long BinaryMessageSerializerPrivate::getMessagePartLength(
        Subscribe& message) /* throws MessageSerializerException */ {
    return getMessagePartLength((ParamIdMessage&) message);
}

void BinaryMessageSerializerPrivate::serialize(Subscribe& message,
        unsigned char* buffer) /* throws MessageSerializerException */ {
    serialize((ParamIdMessage&) message, "Subscribe", &buffer);
}

unsigned long BinaryMessageSerializerPrivate::getMessagePartLength(
        SubscribeResponse& message) {
    return getMessagePartLength((StatusMessage&) message);
}

void BinaryMessageSerializerPrivate::serialize(SubscribeResponse& message,
        unsigned char* buffer) {
    serialize((StatusMessage&) message, "SubscribeResponse", &buffer);
}

unsigned long BinaryMessageSerializerPrivate::getMessagePartLength(
        Unsubscribe& message) /* throws MessageSerializerException */ {
    return getMessagePartLength((ParamIdMessage&) message); // MessageSerializerException
}

void BinaryMessageSerializerPrivate::serialize(Unsubscribe& message,
        unsigned char* buffer) {
    serialize((ParamIdMessage&) message, "Unsubscribe", &buffer);
}

unsigned long BinaryMessageSerializerPrivate::getMessagePartLength(
        UnsubscribeResponse& message) {
    return getMessagePartLength((StatusMessage&) message);
}

void BinaryMessageSerializerPrivate::serialize(UnsubscribeResponse& message,
        unsigned char* buffer) {
    serialize((StatusMessage&) message, "UnsubscribeResponse", &buffer);
}

unsigned long BinaryMessageSerializerPrivate::getMessagePartLength(
        Notification& message) /* throws MessageSerializerException */ {
    return BinaryMessageSerializer::MESSAGE_HEADER_LENGTH + 2 /* noOfParams */
            + getMessagePartLength(message.getParamMap());
}

void BinaryMessageSerializerPrivate::serialize(Notification& message,
        unsigned char* buffer)/* throws MessageSerializerException */ {
    if (isDebugEnabled) {
        logMsg.append("Notification[");
    }
    serialize(message.getMessageHeader(), &buffer);
    if (isDebugEnabled) {
        logMsg.append(",noOfParams=");
    }
    unsigned int noOfParams = message.getParamMap().getElements().size();
    write16(noOfParams, &buffer);
    if (noOfParams > 0) {
        if (isDebugEnabled) {
            logMsg.append(",");
        }
        serialize(message.getParamMap(), &buffer);
    }
    if (isDebugEnabled) {
        logMsg.append("]");
    }
}

unsigned long BinaryMessageSerializerPrivate::getMessagePartLength(
        Event& message) /* throws MessageSerializerException */ {
    // message is specified as string type, thus only length and chars are needed
    return BinaryMessageSerializer::MESSAGE_HEADER_LENGTH
            + 16 /* timeStamp (8 bytes) + severity (4 bytes) 
                  + messageArrayLength (2 bytes) + noOfParams (2 bytes) */
            + getMessagePartLength(message.getEventId())
            + getMessagePartLength(message.getParamId())
            + message.getMessage().length()
            + getMessagePartLength(message.getParamMap());
}

void BinaryMessageSerializerPrivate::serialize(Event& message,
        unsigned char* buffer)/* throws MessageSerializerException */ {
    if (isDebugEnabled) {
        logMsg.append("Event[");
    }
    serialize(message.getMessageHeader(), &buffer);
    if (isDebugEnabled) {
        logMsg.append(",");
    }
    serialize(message.getEventId(), "EventId", &buffer);
    if (isDebugEnabled) {
        logMsg.append(",");
    }
    serialize(message.getParamId(), "ParamId", &buffer);
    if (isDebugEnabled) {
        logMsg.append(",timestamp=");
    }
    write64(message.getTimeStamp(), &buffer);
    if (isDebugEnabled) {
        logMsg.append(",severity=");
    }
    write32(message.getSeverity(), &buffer);
    const std::string& msg = message.getMessage();
    // message is specified as string type, thus only length and chars are needed
    if (isDebugEnabled) {
        logMsg.append(",message=[length=");
    }
    write16((unsigned int) msg.length(), &buffer);
    if (isDebugEnabled) {
        logMsg.append(",values=");
    }
    for (size_t i = 0; i < msg.length(); i++) {
        if (i > 0) {
            if (isDebugEnabled) {
                logMsg.append(" ");
            }
        }
        char c = msg.at(i);
        write8(c, &buffer);
    }
    if (isDebugEnabled) {
        logMsg.append(" (").append(msg).append(")],noOfParams=");
    }
    unsigned int noOfParams = message.getParamMap().getElements().size();
    write16(noOfParams, &buffer);
    if (noOfParams > 0) {
        if (isDebugEnabled) {
            logMsg.append(",");
        }
        serialize(message.getParamMap(), &buffer);
    }
    if (isDebugEnabled) {
        logMsg.append("]");
    }
}

unsigned long BinaryMessageSerializerPrivate::getMessagePartLength(
        Call& message) /* throws MessageSerializerException */ {
    return BinaryMessageSerializer::MESSAGE_HEADER_LENGTH
            + 2 /* noOfParams (2 bytes) */
            + getMessagePartLength(message.getMethodId())
            + getMessagePartLength(message.getParamId())
            + getMessagePartLength(message.getParamList());
}

void BinaryMessageSerializerPrivate::serialize(Call& message,
        unsigned char* buffer)/* throws MessageSerializerException */ {
    if (isDebugEnabled) {
        logMsg.append("Call[");
    }
    serialize(message.getMessageHeader(), &buffer);
    if (isDebugEnabled) {
        logMsg.append(",");
    }
    serialize(message.getMethodId(), "MethodId", &buffer);
    if (isDebugEnabled) {
        logMsg.append(",");
    }
    serialize(message.getParamId(), "ParamId", &buffer);
    if (isDebugEnabled) {
        logMsg.append(",noOfParams=");
    }
    unsigned int noOfParams = message.getParamList().getElements().size();
    write16(noOfParams, &buffer);
    if (noOfParams > 0) {
        if (isDebugEnabled) {
            logMsg.append(",");
        }
        serialize(message.getParamList(), &buffer);
    }
    if (isDebugEnabled) {
        logMsg.append("]");
    }
}

unsigned long BinaryMessageSerializerPrivate::getMessagePartLength(
        CallResponse& message) /* throws MessageSerializerException */ {
    unsigned long len = BinaryMessageSerializer::MESSAGE_HEADER_LENGTH
            + 4 /* status (2 bytes) */;
    switch (message.getStatus()) {
        case Status::SUCCESS:
        case Status::APPLICATION_ERROR:
            len += 2 /* noOfParams (2 bytes) */
                    + getMessagePartLength(*message.getMethodId())
                    + getMessagePartLength(*message.getParamId())
                    + getMessagePartLength(*message.getParamList());
            break;
    }
    return len;
}

void BinaryMessageSerializerPrivate::serialize(CallResponse& message,
        unsigned char* buffer)/* throws MessageSerializerException */ {
    if (isDebugEnabled) {
        logMsg.append("CallResponse[");
    }
    serialize(message.getMessageHeader(), &buffer);
    if (isDebugEnabled) {
        logMsg.append(",status=");
    }
    write16(message.getStatus(), &buffer);
    switch (message.getStatus()) {
        case Status::SUCCESS:
        case Status::APPLICATION_ERROR:
        {
            if (isDebugEnabled) {
                logMsg.append(",");
            }
            serialize(*message.getMethodId(), "MethodId", &buffer);
            if (isDebugEnabled) {
                logMsg.append(",");
            }
            serialize(*message.getParamId(), "ParamId", &buffer);
            if (isDebugEnabled) {
                logMsg.append(",noOfParams=");
            }
            unsigned int noOfParams = message.getParamList()->getElements().size();
            write16(noOfParams, &buffer);
            if (noOfParams > 0) {
                if (isDebugEnabled) {
                    logMsg.append(",");
                }
                serialize(*message.getParamList(), &buffer);
            }
            break;
        }
    }
    if (isDebugEnabled) {
        logMsg.append("]");
    }
}

void BinaryMessageSerializerPrivate::serialize(
        const MessageHeader& messageHeader, unsigned char** bufferPos) {
    if (isDebugEnabled) {
        logMsg.append("MessageHeader[type=");
    }
    write16(messageHeader.getMessageType(), bufferPos);
    if (isDebugEnabled) {
        logMsg.append(",length=");
    }
    write32(BinaryMessageSerializer::MESSAGE_HEADER_LENGTH
            + messageHeader.getMessageBodyLength(), bufferPos);
    if (isDebugEnabled) {
        logMsg.append(",id=");
    }
    write32(messageHeader.getMessageId(), bufferPos);
    if (isDebugEnabled) {
        logMsg.append("]");
    }
}

unsigned long BinaryMessageSerializerPrivate::getMessagePartLength(
        ParamIdMessage& message) /* throws MessageSerializerException */ {
    return BinaryMessageSerializer::MESSAGE_HEADER_LENGTH
            + getMessagePartLength(message.getParamId()); // MessageSerializerException
}

void BinaryMessageSerializerPrivate::serialize(ParamIdMessage& message,
        const char* description,
        unsigned char** bufferPos) /* throws MessageSerializerException */ {
    if (isDebugEnabled) {
        logMsg.append(description).append("[");
    }
    serialize(message.getMessageHeader(), bufferPos);
    if (isDebugEnabled) {
        logMsg.append(",");
    }
    serialize(message.getParamId(), "ParamId", bufferPos);
    if (isDebugEnabled) {
        logMsg.append("]");
    }
}

unsigned long BinaryMessageSerializerPrivate::getMessagePartLength(
        StatusMessage& message) {
    return BinaryMessageSerializer::MESSAGE_HEADER_LENGTH + 2 /* status */;
}

void BinaryMessageSerializerPrivate::serialize(StatusMessage& message,
        const char* description, unsigned char** bufferPos) {
    if (isDebugEnabled) {
        logMsg.append(description).append("[");
    }
    serialize(message.getMessageHeader(), bufferPos);
    if (isDebugEnabled) {
        logMsg.append(",status=");
    }
    write16(message.getStatus(), bufferPos);
    if (isDebugEnabled) {
        logMsg.append("]");
    }
}

unsigned long BinaryMessageSerializerPrivate::getMessagePartLength(
        const ParamId& paramId) /* throws MessageSerializerException */ {
    unsigned long len = 3 /* namespaceIndex (2 byte) + paramType (1 byte) */;
    switch (paramId.getParamIdType()) {
        case ParamId::NUMERIC:
            return len + 4;
        case ParamId::STRING:
            return len + 2 /* length (2 bytes) */
                    + paramId.getString().length();
        default:
            std::ostringstream msg;
            msg << "Unknown paramId type " << paramId.getParamIdType();
            throw ExceptionDef(MessageSerializerException, msg.str());
    }
}

void BinaryMessageSerializerPrivate::serialize(const ParamId& paramId,
        const char* description,
        unsigned char** bufferPos) /* throws MessageSerializerException */ {
    if (isDebugEnabled) {
        logMsg.append(description).append("[type=");
    }
    write16(paramId.getNamespaceIndex(), bufferPos);
    write8(paramId.getParamIdType(), bufferPos);
    switch (paramId.getParamIdType()) {
        case ParamId::NUMERIC:
            if (isDebugEnabled) {
                logMsg.append(",value=");
            }
            write32(paramId.getNumeric(), bufferPos);
            break;
        case ParamId::STRING:
        {
            if (isDebugEnabled) {
                logMsg.append(",length=");
            }
            // length
            size_t len = paramId.getString().length();
            write16((unsigned int) len, bufferPos);
            if (isDebugEnabled) {
                logMsg.append(",values=");
            }
            const char* str = paramId.getString().c_str();
            for (size_t i = 0; i < len; i++) {
                if (isDebugEnabled && i > 0) {
                    logMsg.append(" ");
                }
                write8(str[i], bufferPos);
            }
            if (isDebugEnabled) {
                logMsg.append(" (").append(str).append(")");
            }
            break;
        }
        default:
            std::ostringstream msg;
            msg << "Unknown paramId type " << paramId.getParamIdType();
            throw ExceptionDef(MessageSerializerException, msg.str());
    }
    if (isDebugEnabled) {
        logMsg.append("]");
    }
}

unsigned long BinaryMessageSerializerPrivate::getMessagePartLength(
        const ParamList& paramList) /* throws MessageSerializerException */ {
    const std::vector<const Variant*>& elements = paramList.getElements();
    unsigned long len = 0;
    // paramValue
    for (std::vector<const Variant*>::const_iterator i = elements.begin();
            i != elements.end(); i++) {
        len += 2 /* paramType (2 bytes) */
                + getMessagePartLengthParamValue(**i);
    }
    return len;
}

void BinaryMessageSerializerPrivate::serialize(const ParamList& paramList,
        unsigned char** bufferPos) /* throws MessageSerializerException */ {
    if (isDebugEnabled) {
        logMsg.append("ParamList[");
    }
    const std::vector<const Variant*>& elements = paramList.getElements();
    // for each param
    for (int i = 0; i < elements.size(); i++) {
        const Variant& elem = *elements[i];
        if (isDebugEnabled) {
            if (i > 0) {
                logMsg.append(" ");
            }
            logMsg.append("type=");
        }
        serializeParamTypeValue(elem, bufferPos); // MessageSerializerException
        if (isDebugEnabled) {
            logMsg.append(",value=");
        }
        serializeParamValue(elem, bufferPos);
    }
    if (isDebugEnabled) {
        logMsg.append("]");
    }
}

unsigned long BinaryMessageSerializerPrivate::getMessagePartLength(
        const ParamMap& paramMap)/* throws MessageSerializerException */ {
    const std::map<const ParamId*, const Variant*>& elements =
            paramMap.getElements();
    unsigned long len = 0;
    for (std::map<const ParamId*, const Variant*>::const_iterator i =
            elements.begin(); i != elements.end(); i++) {
        const ParamId& paramId = *(*i).first;
        const Variant& paramValue = *(*i).second;
        len += 2 /* paramType (2 bytes) */
                + getMessagePartLength(paramId)
                + getMessagePartLengthParamValue(paramValue);
    }
    return len;
}

void BinaryMessageSerializerPrivate::serialize(const ParamMap& paramMap,
        unsigned char** bufferPos) /* throws MessageSerializerException */ {
    if (isDebugEnabled) {
        logMsg.append("ParamMap[");
    }
    const std::map<const ParamId*, const Variant*>& elements =
            paramMap.getElements();
    // for each param
    for (std::map<const ParamId*, const Variant*>::const_iterator i =
            elements.begin(); i != elements.end(); i++) {
        const ParamId& paramId = *(*i).first;
        const Variant& paramValue = *(*i).second;
        if (isDebugEnabled && i != elements.begin()) {
            logMsg.append(" ");
        }
        serialize(paramId, "ParamId", bufferPos);
        if (isDebugEnabled) {
            logMsg.append(",type=");
        }
        serializeParamTypeValue(paramValue, bufferPos); // MessageSerializerException
        if (isDebugEnabled) {
            logMsg.append(",value=");
        }
        serializeParamValue(paramValue, bufferPos);
    }
    if (isDebugEnabled) {
        logMsg.append("]");
    }
}

void BinaryMessageSerializerPrivate::serializeParamTypeValue(const Variant& paramValue,
        unsigned char** bufferPos) /* throws MessageSerializerException */ {
    switch (paramValue.getVariantType()) {
        case Variant::SCALAR:
        {
            Scalar& scalar = (Scalar&) paramValue;
            write16(scalar.getScalarType(), bufferPos);
            break;
        }
        case Variant::ARRAY:
        case Variant::STRUCT:
            write16(paramValue.getVariantType(), bufferPos);
            break;
        default:
            std::ostringstream msg;
            msg << "Invalid variant type " << paramValue.getVariantType();
            throw ExceptionDef(MessageSerializerException, msg.str());
    }
}

unsigned long BinaryMessageSerializerPrivate::getMessagePartLengthParamValue(
        const Variant& paramValue) /* throws MessageSerializerException */ {
    switch (paramValue.getVariantType()) {
        case Variant::SCALAR:
            return getMessagePartLengthScalarValue((Scalar&) paramValue); // MessageSerializerException
        case Variant::ARRAY:
            return getMessagePartLengthArrayValue((Array&) paramValue);
        case Variant::STRUCT:
            return getMessagePartLengthStructValue((Struct&) paramValue);
        default:
            std::ostringstream msg;
            msg << "Invalid variant type " << paramValue.getVariantType();
            throw ExceptionDef(MessageSerializerException, msg.str());
    }
}

void BinaryMessageSerializerPrivate::serializeParamValue(const Variant& paramValue,
        unsigned char** bufferPos) /* throws MessageSerializerException */ {
    switch (paramValue.getVariantType()) {
        case Variant::SCALAR:
            serializeScalarValue((Scalar&) paramValue, bufferPos); // MessageSerializerException
            break;
        case Variant::ARRAY:
            serializeArrayValue((Array&) paramValue, bufferPos); // MessageSerializerException
            break;
        case Variant::STRUCT:
            serializeStructValue((Struct&) paramValue, bufferPos); // MessageSerializerException
            break;
        default:
            std::ostringstream msg;
            msg << "Invalid variant type " << paramValue.getVariantType();
            throw ExceptionDef(MessageSerializerException, msg.str());
    }
}

unsigned long BinaryMessageSerializerPrivate::getMessagePartLengthScalarValue(
        const Scalar& value) /* throws MessageSerializerException */ {
    switch (value.getScalarType()) {
        case Scalar::BOOLEAN:
        case Scalar::CHAR:
        case Scalar::BYTE:
            return 1;
        case Scalar::SHORT:
            return 2;
        case Scalar::INT:
        case Scalar::FLOAT:
            return 4;
            break;
        case Scalar::LONG:
        case Scalar::DOUBLE:
            return 8;
        default:
            std::ostringstream msg;
            msg << "Invalid scalar type " << value.getScalarType();
            throw ExceptionDef(MessageSerializerException, msg.str());
    }
}

void BinaryMessageSerializerPrivate::serializeScalarValue(const Scalar& value,
        unsigned char** bufferPos) /* throws MessageSerializerException */ {
    switch (value.getScalarType()) {
        case Scalar::BOOLEAN:
            write8(value.getBoolean(), bufferPos);
            break;
        case Scalar::CHAR:
            write8(value.getChar(), bufferPos);
            break;
        case Scalar::BYTE: // char
            write8(value.getByte(), bufferPos);
            break;
        case Scalar::SHORT: // int
            write16(value.getShort(), bufferPos);
            break;
        case Scalar::INT: // long
            write32(value.getInt(), bufferPos);
            break;
        case Scalar::LONG: // llong
            write64(value.getLong(), bufferPos);
            break;
        case Scalar::FLOAT:
            write32(value.getFloat(), bufferPos);
            break;
        case Scalar::DOUBLE:
            write64(value.getDouble(), bufferPos);
            break;
        default:
            std::ostringstream msg;
            msg << "Unknown scalar type " << value.getScalarType();
            throw ExceptionDef(MessageSerializerException, msg.str());
    }
}

unsigned long BinaryMessageSerializerPrivate::getMessagePartLengthArrayValue(
        const Array& value) /* throws MessageSerializerException */ {
    unsigned long len = 4; // arrayType (2 bytes) + arrayLength (2 bytes)
    // for each element
    for (std::vector<const Variant*>::const_iterator i =
            value.getElements().begin(); i != value.getElements().end(); i++) {
        len += getMessagePartLengthParamValue(**i);
    }
    return len;
}

void BinaryMessageSerializerPrivate::serializeArrayValue(const Array& value,
        unsigned char** bufferPos) /* throws MessageSerializerException */ {
    if (isDebugEnabled) {
        logMsg.append("Array[type=");
    }
    write16(value.getArrayType(), bufferPos);
    if (isDebugEnabled) {
        logMsg.append(",length=");
    }
    const std::vector<const Variant*>& elements = value.getElements();
    write16((unsigned int) elements.size(), bufferPos);
    if (isDebugEnabled) {
        logMsg.append(",values=");
    }
    // for each element
    for (std::vector<const Variant*>::const_iterator it = elements.begin();
            it != elements.end(); it++) {
        if (isDebugEnabled && it != elements.begin()) {
            logMsg.append(" ");
        }
        serializeParamValue(**it, bufferPos); // MessageSerializerException
    }
    if (isDebugEnabled) {
        // if char array
        if (Scalar::CHAR == value.getArrayType()) {
            // append string representation to log message
            char str[elements.size() + 1];
            int i;
            for (i = 0; i < elements.size(); i++) {
                const Scalar* s = static_cast<const Scalar*> (elements[i]);
                str[i] = s->getChar();
            }
            str[i] = '\0';
            logMsg.append(" (").append(str).append(")");
        }
        logMsg.append("]");
    }
}

unsigned long BinaryMessageSerializerPrivate::getMessagePartLengthStructValue(
        const Struct& value) /* throws MessageSerializerException */ {
    unsigned long len = 2 /* fieldCount (2 bytes) */
            + getMessagePartLength(value.getStructId());
    // for each field
    for (std::map<std::string, const Variant*>::const_iterator i = value.getFields().begin();
            i != value.getFields().end(); i++) {
        std::string fieldName = (*i).first;
        const Variant& fieldValue = *(*i).second;
        // field name is specified as string type, thus only length and chars are needed
        len += 4 /* fieldNameArraySize (2 bytes) + fieldType (2 bytes) */
                + fieldName.length()
                + getMessagePartLengthParamValue(fieldValue);
    }
    return len;
}

void BinaryMessageSerializerPrivate::serializeStructValue(const Struct& value,
        unsigned char** bufferPos) /* throws MessageSerializerException */ {
    if (isDebugEnabled) {
        logMsg.append("Struct[");
    }
    serialize(value.getStructId(), "StructId", bufferPos);
    if (isDebugEnabled) {
        logMsg.append(",fieldCount=");
    }
    write16((unsigned int) value.getFields().size(), bufferPos);
    if (isDebugEnabled) {
        logMsg.append(",");
    }
    const std::map<std::string, const Variant*>& fields = value.getFields();
    // for each field
    for (std::map<std::string, const Variant*>::const_iterator it =
            fields.begin(); it != fields.end(); it++) {
        std::string fieldName = (*it).first;
        const Variant& fieldValue = *(*it).second;
        if (isDebugEnabled) {
            if (it != fields.begin()) {
                logMsg.append(" ");
            }
            // field name is specified as string type, thus only length and chars are needed
            logMsg.append("fieldName=[length=");
        }
        write16((unsigned int) fieldName.length(), bufferPos);
        if (isDebugEnabled) {
            logMsg.append(",values=");
        }
        for (size_t j = 0; j < fieldName.length(); j++) {
            if (isDebugEnabled && j > 0) {
                logMsg.append(" ");
            }
            write8(fieldName.at(j), bufferPos);
        }
        if (isDebugEnabled) {
            logMsg.append(" (").append(fieldName).append(")],fieldType=");
        }
        serializeParamTypeValue(fieldValue, bufferPos); // MessageSerializerException
        if (isDebugEnabled) {
            logMsg.append(",fieldValue=");
        }
        serializeParamValue(fieldValue, bufferPos); // MessageSerializerException
    }
    if (isDebugEnabled) {
        logMsg.append("]");
    }
}

void BinaryMessageSerializerPrivate::write8(bool value,
        unsigned char** bufferPos) {
    **bufferPos = value ? 1 : 0;
    if (isDebugEnabled) {
        char* s;
        asprintf(&s, "%s->0x_%x", value ? "true" : "false", **bufferPos);
        logMsg.append(s);
        free(s);
    }
    *bufferPos += charSize;
}

void BinaryMessageSerializerPrivate::write8(char value,
        unsigned char** bufferPos) {
    if (isDebugEnabled) {
        char* s;
        asprintf(&s, "%d/0x%x->", value, value);
        logMsg.append(s);
        free(s);
    }
    write(value, bufferPos, 1);
}

void BinaryMessageSerializerPrivate::write8(int value,
        unsigned char** bufferPos) {
    if (isDebugEnabled) {
        char* s;
        asprintf(&s, "%d/0x%x->", value, value);
        logMsg.append(s);
        free(s);
    }
    write(value, bufferPos, 1);
}

void BinaryMessageSerializerPrivate::write16(int value,
        unsigned char** bufferPos) {
    if (isDebugEnabled) {
        char* s;
        asprintf(&s, "%d/0x%x->", value, value);
        logMsg.append(s);
        free(s);
    }
    write(value, bufferPos, 2);
}

void BinaryMessageSerializerPrivate::write16(unsigned int value,
        unsigned char** bufferPos) {
    if (isDebugEnabled) {
        char* s;
        asprintf(&s, "%u/0x%x->", value, value);
        logMsg.append(s);
        free(s);
    }
    write(value, bufferPos, 2);
}

void BinaryMessageSerializerPrivate::write32(long value,
        unsigned char** bufferPos) {
    if (isDebugEnabled) {
        char* s;
        asprintf(&s, "%ld/0x%lx->", value, value);
        logMsg.append(s);
        free(s);
    }
    write(value, bufferPos, 4);
}

void BinaryMessageSerializerPrivate::write32(unsigned long value,
        unsigned char** bufferPos) {
    if (isDebugEnabled) {
        char* s;
        asprintf(&s, "%lu/0x%lx->", value, value);
        logMsg.append(s);
        free(s);
    }
    write(value, bufferPos, 4);
}

void BinaryMessageSerializerPrivate::write32(float value,
        unsigned char** bufferPos) {
    unsigned long ulongValue = 0;
    if (isBigEndian) {
        memcpy(&ulongValue + (longSize - floatSize), &value, floatSize);
    } else {
        memcpy(&ulongValue, &value, floatSize);
    }
    if (isDebugEnabled) {
        char* s;
        asprintf(&s, "%f/0x%lx->", value, ulongValue);
        logMsg.append(s);
        free(s);
    }
    write(ulongValue, bufferPos, 4);
}

void BinaryMessageSerializerPrivate::write64(long long value,
        unsigned char** bufferPos) {
    if (isDebugEnabled) {
        char* s;
        asprintf(&s, "%lld/0x%llx ->", value, value);
        logMsg.append(s);
        free(s);
    }
    write(value, bufferPos, 8);
}

void BinaryMessageSerializerPrivate::write64(double value,
        unsigned char** bufferPos) {
    unsigned long long ullongValue = 0;
    if (isBigEndian) {
        memcpy(&ullongValue + (llongSize - doubleSize), &value, doubleSize);
    } else {
        memcpy(&ullongValue, &value, doubleSize);
    }
    if (isDebugEnabled) {
        char* s;
        asprintf(&s, "%f/0x%llx ->", value, ullongValue);
        logMsg.append(s);
        free(s);
    }
    write(ullongValue, bufferPos, 8);
}

template<class TYPE>
void BinaryMessageSerializerPrivate::write(TYPE value,
        unsigned char** bufferPos, size_t bufferSize) {
    int cs;
    if (isBigEndian) {
        // append bytes in reverse order to buffer
        *bufferPos += bufferSize - charSize;
        cs = -charSize;
    } else {
        cs = charSize;
    }
    for (int i = bufferSize - charSize; i >= 0; i -= charSize) {
        // append char to buffer
        **bufferPos = (value >> (i * 8)) & charMask;
        *bufferPos += cs;
    }
    if (isBigEndian) {
        *bufferPos += bufferSize + 1;
    }
    if (isDebugEnabled) {
        logMsg.append("0x");
        for (unsigned char* i = *bufferPos - bufferSize; i < *bufferPos; i +=
                charSize) {
            char* s;
            asprintf(&s, "_%x", *i);
            logMsg.append(s);
            free(s);
        }
    }
    // srcValueSize < bufferSize:
    // srcValueSize 2, bufferSize 4, charSize 2
    //   12 34 >> 2
    // ->00 00
    //   12 34 >> 0
    // ->12 34
    // srcValueSize 4, bufferSize 6, charSize 2
    //   00 12 34 56 >> 4
    // ->      00 00
    //   00 12 34 56 >> 2
    // ->      00 12
    //   00 12 34 56 >> 0
    // ->      34 56

    // srcValueSize == bufferSize:
    // srcValueSize 2, bufferSize 2, charSize 2
    //   12 34 >> 0
    // ->12 34
    // srcValueSize 4, bufferSize 4, charSize 2
    //   00 12 34 56 >> 2
    // ->      00 12
    //   00 12 34 56 >> 0
    // ->      34 56

    // srcValueSize > bufferSize:
    // srcValueSize 4, bufferSize 2, charSize 2
    //   00 12 34 56 >> 0
    // ->      34 56
    // srcValueSize 6, bufferSize 4, charSize 2
    //   00 00 12 34 56 >> 2
    // ->         00 12
    //   00 00 12 34 56 >> 0
    // ->         34 56
}
