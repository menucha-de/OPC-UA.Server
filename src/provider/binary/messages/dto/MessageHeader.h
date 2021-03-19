#ifndef PROVIDER_BINARY_MESSAGES_DTO_MESSAGEHEADER_H
#define PROVIDER_BINARY_MESSAGES_DTO_MESSAGEHEADER_H

class MessageHeaderPrivate;

class MessageHeader {
public:

    enum MessageType {
        READ = 0,
        READ_RESPONSE = 1,
        WRITE = 2,
        WRITE_RESPONSE = 3,
        SUBSCRIBE = 4,
        SUBSCRIBE_RESPONSE = 5,
        UNSUBSCRIBE = 6,
        UNSUBSCRIBE_RESPONSE = 7,
        NOTIFICATION = 8,
        EVENT = 9,
        CALL = 10,
        CALL_RESPONSE = 11
    };

    MessageHeader(MessageType messageType, unsigned long messageId);
    // Creates a deep copy of the instance.
    MessageHeader(const MessageHeader& src);    
    virtual ~MessageHeader();

    virtual MessageType getMessageType() const; // 16 bit
    // The length of the message body (32 bit)
    virtual unsigned long getMessageBodyLength() const;
    virtual void setMessageBodyLength(unsigned long messageBodyLength);
    // A correlation identifier for a request message and its response (32 bit)
    virtual unsigned long getMessageId() const;
private:    
    MessageHeader& operator=(const MessageHeader& orig);
    
    MessageHeaderPrivate* d;
};

#endif /* PROVIDER_BINARY_MESSAGES_DTO_MESSAGEHEADER_H */

