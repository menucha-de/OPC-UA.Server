#include "CppUTest/TestHarness.h"
#include "../../../Env.h"
#include "../../../../../src/provider/binary/common/ClientSocket.h"
#include "../../../../../src/provider/binary/common/ServerSocket.h"
#include "../../../../../src/provider/binary/common/ServerSocketException.h"
#include "../../../../../src/provider/binary/common/TimeoutException.h"
#include "../../../../../src/provider/binary/ioDataProvider/BinaryIODataProvider.h"
#include "../../../../../src/provider/binary/messages/BinaryMessageDeserializer.h"
#include "../../../../../src/provider/binary/messages/BinaryMessageSerializer.h"
#include "../../../../../src/provider/binary/messages/dto/Array.h"
#include "../../../../../src/provider/binary/messages/dto/Call.h"
#include "../../../../../src/provider/binary/messages/dto/CallResponse.h"
#include "../../../../../src/provider/binary/messages/dto/Event.h"
#include "../../../../../src/provider/binary/messages/dto/MessageHeader.h"
#include "../../../../../src/provider/binary/messages/dto/Notification.h"
#include "../../../../../src/provider/binary/messages/dto/ParamId.h"
#include "../../../../../src/provider/binary/messages/dto/Read.h"
#include "../../../../../src/provider/binary/messages/dto/ReadResponse.h"
#include "../../../../../src/provider/binary/messages/dto/Scalar.h"
#include "../../../../../src/provider/binary/messages/dto/Struct.h"
#include "../../../../../src/provider/binary/messages/dto/Subscribe.h"
#include "../../../../../src/provider/binary/messages/dto/SubscribeResponse.h"
#include "../../../../../src/provider/binary/messages/dto/Unsubscribe.h"
#include "../../../../../src/provider/binary/messages/dto/UnsubscribeResponse.h"
#include "../../../../../src/provider/binary/messages/dto/Write.h"
#include "../../../../../src/provider/binary/messages/dto/WriteResponse.h"
#include <common/ScopeGuard.h>
#include <common/VectorScopeGuard.h>
#include <common/logging/ConsoleLoggerFactory.h>
#include <common/logging/LoggerFactory.h>
#include <ioDataProvider/Array.h>
#include <ioDataProvider/Event.h>
#include <ioDataProvider/OpcUaEventData.h>
#include <ioDataProvider/IODataProviderException.h>
#include <ioDataProvider/Structure.h>
#include <ioDataProvider/SubscriberCallback.h>
#include <string>
#include <pthread.h> // pthread_t
#include <time.h> //nanosleep

using namespace CommonNamespace;

namespace TestNamespace {

    TEST_GROUP(ProviderBinaryIoDataProvider_BinaryIODataProvider) {
        ConsoleLoggerFactory clf;
        LoggerFactory* lf;

        void setup() {
            lf = new LoggerFactory(clf);
        }

        void teardown() {
            delete lf;
        }

        class SubscribeSubscriberCallback : public IODataProviderNamespace::SubscriberCallback {
        public:
            std::vector<IODataProviderNamespace::Event*> events;

            SubscribeSubscriberCallback() {
            }

            virtual ~SubscribeSubscriberCallback() {
                // delete received events
                for (std::vector<IODataProviderNamespace::Event*>::iterator i = events.begin();
                        i != events.end(); i++) {
                    delete *i;
                }
            }

            virtual void valuesChanged(const IODataProviderNamespace::Event& event) {
                if (events.size() == 2) {
                    FAIL("");
                }
                // save a copy of the event
                events.push_back(new IODataProviderNamespace::Event(event));
            }
        };

        static void* openCloseThreadRun(void* object) {
            ServerSocket& serverSocket = *(ServerSocket*) object;
            unsigned char serverReadBuffer[3];
            try {
                serverSocket.readData(3, serverReadBuffer);
                FAIL("");
            } catch (ServerSocketException& e) {
                STRCMP_CONTAINS("Connection closed by remote side", e.getMessage().c_str());
            }
        }

        static Message * readMessage(ServerSocket & serverSocket) {
            unsigned char bufferHeader[BinaryMessageSerializer::MESSAGE_HEADER_LENGTH];
            serverSocket.readData(BinaryMessageSerializer::MESSAGE_HEADER_LENGTH, bufferHeader);
            BinaryMessageDeserializer deserializer;
            MessageHeader* messageHeader =
                    deserializer.deserializeMessageHeader(bufferHeader);
            ScopeGuard<MessageHeader> messageHeaderSG(messageHeader);
            size_t bufferSize = messageHeader->getMessageBodyLength();
            unsigned char bufferBody[bufferSize];
            serverSocket.readData(bufferSize, bufferBody);
            return deserializer.deserializeMessageBody(*messageHeader,
                    bufferBody);
        }

        static void sendMessage(ServerSocket & serverSocket, Message * message) {
            BinaryMessageSerializer serializer;
            unsigned char* responseBuffer = serializer.serialize(*message);
            ScopeGuard<unsigned char> responseBufferSG(responseBuffer, true /*isArray*/);
            serverSocket.writeData(BinaryMessageSerializer::MESSAGE_HEADER_LENGTH
                    + message->getMessageHeader().getMessageBodyLength(), responseBuffer);
        }

        static void* readThreadRun(void* object) {
            ServerSocket& serverSocket = *(ServerSocket*) object;
            // send responses for read requests
            // 1. request (boolean)
            {
                Read& readRequest = *static_cast<Read*> (readMessage(serverSocket));
                ScopeGuard<Read> readRequestSG(&readRequest);
                ReadResponse readResponse(readRequest.getMessageHeader().getMessageId(),
                        Status::SUCCESS);
                readResponse.setParamId(&readRequest.getParamId());
                Scalar paramValue;
                paramValue.setBoolean(true);
                readResponse.setParamValue(&paramValue);
                sendMessage(serverSocket, &readResponse);
            }
            // 2. request (char)
            {
                Read& readRequest = *static_cast<Read*> (readMessage(serverSocket));
                ScopeGuard<Read> readRequestSG(&readRequest);
                ReadResponse readResponse(readRequest.getMessageHeader().getMessageId(),
                        Status::SUCCESS);
                readResponse.setParamId(&readRequest.getParamId());
                Scalar paramValue;
                paramValue.setChar('a');
                readResponse.setParamValue(&paramValue);
                sendMessage(serverSocket, &readResponse);
            }
            // 3. request (byte -> schar)
            {
                Read& readRequest = *static_cast<Read*> (readMessage(serverSocket));
                ScopeGuard<Read> readRequestSG(&readRequest);
                ReadResponse readResponse(readRequest.getMessageHeader().getMessageId(),
                        Status::SUCCESS);
                readResponse.setParamId(&readRequest.getParamId());
                Scalar paramValue;
                paramValue.setByte((signed char) 33);
                readResponse.setParamValue(&paramValue);
                sendMessage(serverSocket, &readResponse);
            }
            // 4. request (short -> int)
            {
                Read& readRequest = *static_cast<Read*> (readMessage(serverSocket));
                ScopeGuard<Read> readRequestSG(&readRequest);
                ReadResponse readResponse(readRequest.getMessageHeader().getMessageId(),
                        Status::SUCCESS);
                readResponse.setParamId(&readRequest.getParamId());
                Scalar paramValue;
                paramValue.setShort((short) 44);
                readResponse.setParamValue(&paramValue);
                sendMessage(serverSocket, &readResponse);
            }
            // 5. request (int -> long)
            {
                Read& readRequest = *static_cast<Read*> (readMessage(serverSocket));
                ScopeGuard<Read> readRequestSG(&readRequest);
                ReadResponse readResponse(readRequest.getMessageHeader().getMessageId(),
                        Status::SUCCESS);
                readResponse.setParamId(&readRequest.getParamId());
                Scalar paramValue;
                paramValue.setInt(55);
                readResponse.setParamValue(&paramValue);
                sendMessage(serverSocket, &readResponse);
            }
            // 6. request (long -> llong)
            {
                Read& readRequest = *static_cast<Read*> (readMessage(serverSocket));
                ScopeGuard<Read> readRequestSG(&readRequest);
                ReadResponse readResponse(readRequest.getMessageHeader().getMessageId(),
                        Status::SUCCESS);
                readResponse.setParamId(&readRequest.getParamId());
                Scalar paramValue;
                paramValue.setLong(66);
                readResponse.setParamValue(&paramValue);
                sendMessage(serverSocket, &readResponse);
            }
            // 7. request (float)
            {
                Read& readRequest = *static_cast<Read*> (readMessage(serverSocket));
                ScopeGuard<Read> readRequestSG(&readRequest);
                ReadResponse readResponse(readRequest.getMessageHeader().getMessageId(),
                        Status::SUCCESS);
                readResponse.setParamId(&readRequest.getParamId());
                Scalar paramValue;
                paramValue.setFloat(7.7);
                readResponse.setParamValue(&paramValue);
                sendMessage(serverSocket, &readResponse);
            }
            // 8. request (double)
            {
                Read& readRequest = *static_cast<Read*> (readMessage(serverSocket));
                ScopeGuard<Read> readRequestSG(&readRequest);
                ReadResponse readResponse(readRequest.getMessageHeader().getMessageId(),
                        Status::SUCCESS);
                readResponse.setParamId(&readRequest.getParamId());
                Scalar paramValue;
                paramValue.setDouble(8.8);
                readResponse.setParamValue(&paramValue);
                sendMessage(serverSocket, &readResponse);
            }
            // 9. request (structure)
            {
                Read& readRequest = *static_cast<Read*> (readMessage(serverSocket));
                ScopeGuard<Read> readRequestSG(&readRequest);
                ReadResponse readResponse(readRequest.getMessageHeader().getMessageId(),
                        Status::SUCCESS);
                readResponse.setParamId(&readRequest.getParamId());
                ParamId structId(110);
                std::map<std::string, const Variant*> fields;

                Scalar fieldValue1;
                fieldValue1.setInt(1111);
                fields[std::string("x1")] = &fieldValue1;

                Scalar fieldValue2;
                fieldValue2.setInt(1112);
                fields[std::string("x2")] = &fieldValue2;

                Struct paramValue(structId, fields);
                readResponse.setParamValue(&paramValue);
                sendMessage(serverSocket, &readResponse);
            }
            // 10. request (char[] -> string)
            {
                Read& readRequest = *static_cast<Read*> (readMessage(serverSocket));
                ScopeGuard<Read> readRequestSG(&readRequest);
                ReadResponse readResponse(readRequest.getMessageHeader().getMessageId(),
                        Status::SUCCESS);
                readResponse.setParamId(&readRequest.getParamId());
                std::vector<const Variant*> paramValueElems;
                Scalar paramValueElem1;
                paramValueElem1.setChar('a');
                paramValueElems.push_back(&paramValueElem1);
                Scalar paramValueElem2;
                paramValueElem2.setChar('b');
                paramValueElems.push_back(&paramValueElem2);
                Array paramValue(Scalar::CHAR, paramValueElems);
                readResponse.setParamValue(&paramValue);
                sendMessage(serverSocket, &readResponse);
            }
            // 11. request (byte[] -> byteString)
            {
                Read& readRequest = *static_cast<Read*> (readMessage(serverSocket));
                ScopeGuard<Read> readRequestSG(&readRequest);
                ReadResponse readResponse(readRequest.getMessageHeader().getMessageId(),
                        Status::SUCCESS);
                readResponse.setParamId(&readRequest.getParamId());
                std::vector<const Variant*> paramValueElems;
                Scalar paramValueElem1;
                paramValueElem1.setByte(34);
                paramValueElems.push_back(&paramValueElem1);
                Scalar paramValueElem2;
                paramValueElem2.setByte(255);
                paramValueElems.push_back(&paramValueElem2);
                Array paramValue(Scalar::BYTE, paramValueElems);
                readResponse.setParamValue(&paramValue);
                sendMessage(serverSocket, &readResponse);
            }
            // 12. request (structure[])
            {
                Read& readRequest = *static_cast<Read*> (readMessage(serverSocket));
                ScopeGuard<Read> readRequestSG(&readRequest);
                ReadResponse readResponse(readRequest.getMessageHeader().getMessageId(),
                        Status::SUCCESS);
                readResponse.setParamId(&readRequest.getParamId());

                ParamId structId1(1111);
                std::map<std::string, const Variant*> fields1;
                Scalar fieldValue1;
                fieldValue1.setInt(1121);
                fields1[std::string("x1")] = &fieldValue1;
                Struct struc1(structId1, fields1);

                ParamId structId2(1112);
                std::map<std::string, const Variant*> fields2;
                Scalar fieldValue2;
                fieldValue2.setInt(1122);
                fields2[std::string("x2")] = &fieldValue2;
                Struct struc2(structId2, fields2);

                std::vector<const Variant*> arrayElements;
                arrayElements.push_back(&struc1);
                arrayElements.push_back(&struc2);
                Array paramValue(Variant::STRUCT, arrayElements);
                readResponse.setParamValue(&paramValue);
                sendMessage(serverSocket, &readResponse);
            }
            // 13. request (char[][] -> string[])
            {
                Read& readRequest = *static_cast<Read*> (readMessage(serverSocket));
                ScopeGuard<Read> readRequestSG(&readRequest);
                ReadResponse readResponse(readRequest.getMessageHeader().getMessageId(),
                        Status::SUCCESS);
                readResponse.setParamId(&readRequest.getParamId());
                // 1. char[]
                std::vector<const Variant*> paramValueElems1;
                Scalar paramValueElem11;
                paramValueElem11.setChar('a');
                paramValueElems1.push_back(&paramValueElem11);
                Scalar paramValueElem12;
                paramValueElem12.setChar('b');
                paramValueElems1.push_back(&paramValueElem12);
                Array paramValue1(Scalar::CHAR, paramValueElems1);
                // 2. char[]
                std::vector<const Variant*> paramValueElems2;
                Scalar paramValueElem21;
                paramValueElem21.setChar('c');
                paramValueElems2.push_back(&paramValueElem21);
                Scalar paramValueElem22;
                paramValueElem22.setChar('d');
                paramValueElems2.push_back(&paramValueElem22);
                Array paramValue2(Scalar::CHAR, paramValueElems2);
                // char[][]
                std::vector<const Variant*> paramValueElems;
                paramValueElems.push_back(&paramValue1);
                paramValueElems.push_back(&paramValue2);
                Array paramValue(Variant::ARRAY, paramValueElems);
                readResponse.setParamValue(&paramValue);
                sendMessage(serverSocket, &readResponse);
            }
        }

        static void* readErrorsThreadRun(void* object) {
            ServerSocket& serverSocket = *(ServerSocket*) object;
            // send a response with an error code
            Read& readRequest = *static_cast<Read*> (readMessage(serverSocket));
            ScopeGuard<Read> readRequestSG(&readRequest);
            ReadResponse readResponse(readRequest.getMessageHeader().getMessageId(),
                    Status::INVALID_MESSAGE);
            sendMessage(serverSocket, &readResponse);
        }

        static void* writeThreadRun(void* object) {
            ServerSocket& serverSocket = *(ServerSocket*) object;
            // send responses for read requests:
            // bool
            {
                Write& writeRequest = *static_cast<Write*> (readMessage(serverSocket));
                ScopeGuard<Write> writeRequestSG(&writeRequest);
                STRCMP_EQUAL("11", writeRequest.getParamId().getString().c_str());
                const Scalar& paramValue = static_cast<const Scalar&> (writeRequest.getParamValue());
                CHECK_TRUE(paramValue.getBoolean());
                WriteResponse writeResponse(writeRequest.getMessageHeader().getMessageId(),
                        Status::SUCCESS);
                sendMessage(serverSocket, &writeResponse);
            }
            // byte
            {
                Write& writeRequest = *static_cast<Write*> (readMessage(serverSocket));
                ScopeGuard<Write> writeRequestSG(&writeRequest);
                CHECK_EQUAL(12, writeRequest.getParamId().getNumeric());
                const Scalar& paramValue = static_cast<const Scalar&> (writeRequest.getParamValue());
                CHECK_EQUAL(0x22, paramValue.getByte());
                WriteResponse writeResponse(writeRequest.getMessageHeader().getMessageId(),
                        Status::SUCCESS);
                sendMessage(serverSocket, &writeResponse);
            }
            // char
            {
                Write& writeRequest = *static_cast<Write*> (readMessage(serverSocket));
                ScopeGuard<Write> writeRequestSG(&writeRequest);
                CHECK_EQUAL(13, writeRequest.getParamId().getNumeric());
                const Scalar& paramValue = static_cast<const Scalar&> (writeRequest.getParamValue());
                CHECK_EQUAL('a', paramValue.getChar());
                WriteResponse writeResponse(writeRequest.getMessageHeader().getMessageId(),
                        Status::SUCCESS);
                sendMessage(serverSocket, &writeResponse);
            }
            // int -> short
            {
                Write& writeRequest = *static_cast<Write*> (readMessage(serverSocket));
                ScopeGuard<Write> writeRequestSG(&writeRequest);
                CHECK_EQUAL(14, writeRequest.getParamId().getNumeric());
                const Scalar& paramValue = static_cast<const Scalar&> (writeRequest.getParamValue());
                CHECK_EQUAL(44, paramValue.getShort());
                WriteResponse writeResponse(writeRequest.getMessageHeader().getMessageId(),
                        Status::SUCCESS);
                sendMessage(serverSocket, &writeResponse);
            }
            // long -> int
            {
                Write& writeRequest = *static_cast<Write*> (readMessage(serverSocket));
                ScopeGuard<Write> writeRequestSG(&writeRequest);
                CHECK_EQUAL(15, writeRequest.getParamId().getNumeric());
                const Scalar& paramValue = static_cast<const Scalar&> (writeRequest.getParamValue());
                CHECK_EQUAL(55, paramValue.getInt());
                WriteResponse writeResponse(writeRequest.getMessageHeader().getMessageId(),
                        Status::SUCCESS);
                sendMessage(serverSocket, &writeResponse);
            }
            // llong -> long
            {
                Write& writeRequest = *static_cast<Write*> (readMessage(serverSocket));
                ScopeGuard<Write> writeRequestSG(&writeRequest);
                CHECK_EQUAL(16, writeRequest.getParamId().getNumeric());
                const Scalar& paramValue = static_cast<const Scalar&> (writeRequest.getParamValue());
                LONGS_EQUAL(66L, paramValue.getLong());
                WriteResponse writeResponse(writeRequest.getMessageHeader().getMessageId(),
                        Status::SUCCESS);
                sendMessage(serverSocket, &writeResponse);
            }
            // float
            {
                Write& writeRequest = *static_cast<Write*> (readMessage(serverSocket));
                ScopeGuard<Write> writeRequestSG(&writeRequest);
                CHECK_EQUAL(17, writeRequest.getParamId().getNumeric());
                const Scalar& paramValue = static_cast<const Scalar&> (writeRequest.getParamValue());
                DOUBLES_EQUAL(7.7, paramValue.getFloat(), 0.01);
                WriteResponse writeResponse(writeRequest.getMessageHeader().getMessageId(),
                        Status::SUCCESS);
                sendMessage(serverSocket, &writeResponse);
            }
            // double
            {
                Write& writeRequest = *static_cast<Write*> (readMessage(serverSocket));
                ScopeGuard<Write> writeRequestSG(&writeRequest);
                CHECK_EQUAL(18, writeRequest.getParamId().getNumeric());
                const Scalar& paramValue = static_cast<const Scalar&> (writeRequest.getParamValue());
                DOUBLES_EQUAL(8.8, paramValue.getDouble(), 0.01);
                WriteResponse writeResponse(writeRequest.getMessageHeader().getMessageId(),
                        Status::SUCCESS);
                sendMessage(serverSocket, &writeResponse);
            }
            // string -> char[]
            {
                Write& writeRequest = *static_cast<Write*> (readMessage(serverSocket));
                ScopeGuard<Write> writeRequestSG(&writeRequest);
                CHECK_EQUAL(19, writeRequest.getParamId().getNumeric());
                Array& paramValue = (Array&) writeRequest.getParamValue();
                CHECK_EQUAL(Scalar::CHAR, paramValue.getArrayType());
                const std::vector<const Variant*>& elems = paramValue.getElements();
                CHECK_EQUAL(2, elems.size());
                const Scalar& elemselem1 = *static_cast<const Scalar*> (elems.at(0));
                CHECK_EQUAL('a', elemselem1.getChar());
                const Scalar& elemselem2 = *static_cast<const Scalar*> (elems.at(1));
                CHECK_EQUAL('b', elemselem2.getChar());
                WriteResponse writeResponse(writeRequest.getMessageHeader().getMessageId(),
                        Status::SUCCESS);
                sendMessage(serverSocket, &writeResponse);
            }
            {
                Write& writeRequest = *static_cast<Write*> (readMessage(serverSocket));
                ScopeGuard<Write> writeRequestSG(&writeRequest);
                CHECK_EQUAL(191, writeRequest.getParamId().getNumeric());
                Array& paramValue = (Array&) writeRequest.getParamValue();
                CHECK_EQUAL(Scalar::CHAR, paramValue.getArrayType());
                const std::vector<const Variant*>& elems = paramValue.getElements();
                CHECK_EQUAL(0, elems.size());
                WriteResponse writeResponse(writeRequest.getMessageHeader().getMessageId(),
                        Status::SUCCESS);
                sendMessage(serverSocket, &writeResponse);
            }
            // structure
            {
                Write& writeRequest = *static_cast<Write*> (readMessage(serverSocket));
                ScopeGuard<Write> writeRequestSG(&writeRequest);
                CHECK_EQUAL(20, writeRequest.getParamId().getNumeric());
                const Struct& paramValue = static_cast<const Struct&> (writeRequest.getParamValue());
                CHECK_EQUAL(110, paramValue.getStructId().getNumeric());
                CHECK_EQUAL(2, paramValue.getFields().size());

                const Scalar& fieldValue1 = *static_cast<const Scalar*> (
                        paramValue.getFields().at(std::string("x1")));
                LONGS_EQUAL(1111, fieldValue1.getInt());

                const Scalar& fieldValue2 = *static_cast<const Scalar*> (
                        paramValue.getFields().at(std::string("x2")));
                LONGS_EQUAL(1112, fieldValue2.getInt());

                WriteResponse writeResponse(writeRequest.getMessageHeader().getMessageId(),
                        Status::SUCCESS);
                sendMessage(serverSocket, &writeResponse);
            }
            // array of structure
            {
                Write& writeRequest = *static_cast<Write*> (readMessage(serverSocket));
                ScopeGuard<Write> writeRequestSG(&writeRequest);
                CHECK_EQUAL(21, writeRequest.getParamId().getNumeric());
                const Array& arrayValue = static_cast<const Array&> (writeRequest.getParamValue());
                CHECK_EQUAL(2, arrayValue.getElements().size());

                const Struct& structValue1 = *static_cast<const Struct*> (arrayValue.getElements().at(0));
                CHECK_EQUAL(1111, structValue1.getStructId().getNumeric());
                CHECK_EQUAL(1, structValue1.getFields().size());
                const Scalar& fieldValue1 = *static_cast<const Scalar*> (
                        structValue1.getFields().at(std::string("x1")));
                CHECK_EQUAL(1121, fieldValue1.getInt());

                const Struct& structValue2 = *static_cast<const Struct*> (arrayValue.getElements().at(1));
                CHECK_EQUAL(1112, structValue2.getStructId().getNumeric());
                CHECK_EQUAL(1, structValue2.getFields().size());
                const Scalar& fieldValue2 = *static_cast<const Scalar*> (
                        structValue2.getFields().at(std::string("x2")));
                CHECK_EQUAL(1122, fieldValue2.getInt());

                WriteResponse writeResponse(writeRequest.getMessageHeader().getMessageId(),
                        Status::SUCCESS);
                sendMessage(serverSocket, &writeResponse);
            }
            // string -> byte[]
            {
                Write& writeRequest = *static_cast<Write*> (readMessage(serverSocket));
                ScopeGuard<Write> writeRequestSG(&writeRequest);
                CHECK_EQUAL(22, writeRequest.getParamId().getNumeric());
                Array& paramValue = (Array&) writeRequest.getParamValue();
                CHECK_EQUAL(Scalar::BYTE, paramValue.getArrayType());
                const std::vector<const Variant*>& elems = paramValue.getElements();
                CHECK_EQUAL(2, elems.size());
                const Scalar& elemselem1 = *static_cast<const Scalar*> (elems.at(0));
                CHECK_EQUAL(34, elemselem1.getByte() & 0xFF);
                const Scalar& elemselem2 = *static_cast<const Scalar*> (elems.at(1));
                CHECK_EQUAL(255, elemselem2.getByte() & 0xFF);
                WriteResponse writeResponse(writeRequest.getMessageHeader().getMessageId(),
                        Status::SUCCESS);
                sendMessage(serverSocket, &writeResponse);
            }
            {
                Write& writeRequest = *static_cast<Write*> (readMessage(serverSocket));
                ScopeGuard<Write> writeRequestSG(&writeRequest);
                CHECK_EQUAL(23, writeRequest.getParamId().getNumeric());
                Array& paramValue = (Array&) writeRequest.getParamValue();
                CHECK_EQUAL(Scalar::BYTE, paramValue.getArrayType());
                const std::vector<const Variant*>& elems = paramValue.getElements();
                CHECK_EQUAL(0, elems.size());
                WriteResponse writeResponse(writeRequest.getMessageHeader().getMessageId(),
                        Status::SUCCESS);
                sendMessage(serverSocket, &writeResponse);
            }
            // localizedText -> char[]
            {
                Write& writeRequest = *static_cast<Write*> (readMessage(serverSocket));
                ScopeGuard<Write> writeRequestSG(&writeRequest);
                CHECK_EQUAL(24, writeRequest.getParamId().getNumeric());
                Array& paramValue = (Array&) writeRequest.getParamValue();
                CHECK_EQUAL(Scalar::CHAR, paramValue.getArrayType());
                const std::vector<const Variant*>& elems = paramValue.getElements();
                CHECK_EQUAL(2, elems.size());
                const Scalar& elemselem1 = *static_cast<const Scalar*> (elems.at(0));
                CHECK_EQUAL('a', elemselem1.getChar());
                const Scalar& elemselem2 = *static_cast<const Scalar*> (elems.at(1));
                CHECK_EQUAL('b', elemselem2.getChar());
                WriteResponse writeResponse(writeRequest.getMessageHeader().getMessageId(),
                        Status::SUCCESS);
                sendMessage(serverSocket, &writeResponse);
            }
            {
                Write& writeRequest = *static_cast<Write*> (readMessage(serverSocket));
                ScopeGuard<Write> writeRequestSG(&writeRequest);
                CHECK_EQUAL(25, writeRequest.getParamId().getNumeric());
                Array& paramValue = (Array&) writeRequest.getParamValue();
                CHECK_EQUAL(Scalar::CHAR, paramValue.getArrayType());
                const std::vector<const Variant*>& elems = paramValue.getElements();
                CHECK_EQUAL(0, elems.size());
                WriteResponse writeResponse(writeRequest.getMessageHeader().getMessageId(),
                        Status::SUCCESS);
                sendMessage(serverSocket, &writeResponse);
            }
            // string[] -> char[][]
            {
                Write& writeRequest = *static_cast<Write*> (readMessage(serverSocket));
                ScopeGuard<Write> writeRequestSG(&writeRequest);
                CHECK_EQUAL(26, writeRequest.getParamId().getNumeric());
                Array& paramValue = (Array&) writeRequest.getParamValue();
                CHECK_EQUAL(Variant::ARRAY, paramValue.getArrayType());
                CHECK_EQUAL(2, paramValue.getElements().size());

                const Array* a = static_cast<const Array*> (paramValue.getElements().at(0));
                CHECK_EQUAL(Scalar::CHAR, a->getArrayType());
                CHECK_EQUAL(2, a->getElements().size());
                CHECK_EQUAL('a', static_cast<const Scalar*> (a->getElements().at(0))->getChar());
                CHECK_EQUAL('b', static_cast<const Scalar*> (a->getElements().at(1))->getChar());
                a = static_cast<const Array*> (paramValue.getElements().at(1));
                CHECK_EQUAL(Scalar::CHAR, a->getArrayType());
                CHECK_EQUAL(2, a->getElements().size());
                CHECK_EQUAL('c', static_cast<const Scalar*> (a->getElements().at(0))->getChar());
                CHECK_EQUAL('d', static_cast<const Scalar*> (a->getElements().at(1))->getChar());

                WriteResponse writeResponse(writeRequest.getMessageHeader().getMessageId(),
                        Status::SUCCESS);
                sendMessage(serverSocket, &writeResponse);
            }
            // byteString[] -> byte[][]
            {
                Write& writeRequest = *static_cast<Write*> (readMessage(serverSocket));
                ScopeGuard<Write> writeRequestSG(&writeRequest);
                CHECK_EQUAL(27, writeRequest.getParamId().getNumeric());
                Array& paramValue = (Array&) writeRequest.getParamValue();
                CHECK_EQUAL(Variant::ARRAY, paramValue.getArrayType());
                CHECK_EQUAL(2, paramValue.getElements().size());

                const Array* a = static_cast<const Array*> (paramValue.getElements().at(0));
                CHECK_EQUAL(Scalar::BYTE, a->getArrayType());
                CHECK_EQUAL(2, a->getElements().size());
                CHECK_EQUAL('a', static_cast<const Scalar*> (a->getElements().at(0))->getByte());
                CHECK_EQUAL('b', static_cast<const Scalar*> (a->getElements().at(1))->getByte());
                a = static_cast<const Array*> (paramValue.getElements().at(1));
                CHECK_EQUAL(Scalar::BYTE, a->getArrayType());
                CHECK_EQUAL(2, a->getElements().size());
                CHECK_EQUAL('c', static_cast<const Scalar*> (a->getElements().at(0))->getByte());
                CHECK_EQUAL('d', static_cast<const Scalar*> (a->getElements().at(1))->getByte());

                WriteResponse writeResponse(writeRequest.getMessageHeader().getMessageId(),
                        Status::SUCCESS);
                sendMessage(serverSocket, &writeResponse);
            }
            // localizedText[] -> char[][]
            {
                Write& writeRequest = *static_cast<Write*> (readMessage(serverSocket));
                ScopeGuard<Write> writeRequestSG(&writeRequest);
                CHECK_EQUAL(28, writeRequest.getParamId().getNumeric());
                Array& paramValue = (Array&) writeRequest.getParamValue();
                CHECK_EQUAL(Variant::ARRAY, paramValue.getArrayType());
                CHECK_EQUAL(2, paramValue.getElements().size());

                const Array* a = static_cast<const Array*> (paramValue.getElements().at(0));
                CHECK_EQUAL(Scalar::CHAR, a->getArrayType());
                CHECK_EQUAL(2, a->getElements().size());
                CHECK_EQUAL('a', static_cast<const Scalar*> (a->getElements().at(0))->getChar());
                CHECK_EQUAL('b', static_cast<const Scalar*> (a->getElements().at(1))->getChar());
                a = static_cast<const Array*> (paramValue.getElements().at(1));
                CHECK_EQUAL(Scalar::CHAR, a->getArrayType());
                CHECK_EQUAL(2, a->getElements().size());
                CHECK_EQUAL('c', static_cast<const Scalar*> (a->getElements().at(0))->getChar());
                CHECK_EQUAL('d', static_cast<const Scalar*> (a->getElements().at(1))->getChar());

                WriteResponse writeResponse(writeRequest.getMessageHeader().getMessageId(),
                        Status::SUCCESS);
                sendMessage(serverSocket, &writeResponse);
            }
        }

        static void* writeErrorsThreadRun(void* object) {
            ServerSocket& serverSocket = *(ServerSocket*) object;
            // send a response with an error code
            Write& writeRequest = *static_cast<Write*> (readMessage(serverSocket));
            ScopeGuard<Write> writeRequestSG(&writeRequest);
            WriteResponse writeResponse(writeRequest.getMessageHeader().getMessageId(),
                    Status::INVALID_MESSAGE);
            sendMessage(serverSocket, &writeResponse);
        }

        static void* callThreadRun(void* object) {
            ServerSocket& serverSocket = *(ServerSocket*) object;
            // send responses for call requests            
            // 1. request (APPLICATION ERROR)
            {
                Call* callRequest = static_cast<Call*> (readMessage(serverSocket));
                ScopeGuard<Call> callRequestSG(callRequest);
                ParamId methodId(callRequest->getMethodId());
                ParamId objectId(callRequest->getParamId());
                std::vector<const Variant*> outElems;
                const std::vector<const Variant*>& inElems = callRequest->getParamList().getElements();

                Scalar e1;
                e1.setInt(static_cast<const Scalar*> (inElems[0])->getLong() + 3);
                outElems.push_back(&e1);

                std::vector<const Variant*> elems;
                const Array& inArray = *static_cast<const Array*> (inElems[1]);
                elems.push_back(inArray.getElements().at(0));
                Scalar elem2;
                elem2.setChar('3');
                elems.push_back(&elem2);
                Array e2(Scalar::CHAR, elems);
                outElems.push_back(&e2);

                ParamList paramList(outElems);
                CallResponse callResponse(callRequest->getMessageHeader().getMessageId(),
                        Status::APPLICATION_ERROR);
                callResponse.setMethodId(&methodId);
                callResponse.setParamId(&objectId);
                callResponse.setParamList(&paramList);
                sendMessage(serverSocket, &callResponse);
            }
            // 2. request (SUCCESS)
            {
                Call* callRequest = static_cast<Call*> (readMessage(serverSocket));
                ScopeGuard<Call> callRequestSG(callRequest);
                ParamId methodId(callRequest->getMethodId());
                ParamId paramId(callRequest->getParamId());
                std::vector<const Variant*> outElems;
                const std::vector<const Variant*>& inElems = callRequest->getParamList().getElements();
                Scalar e1;
                e1.setInt(((Scalar*) inElems[0])->getInt() + 1);
                outElems.push_back(&e1);
                Scalar e2;
                e2.setFloat(((Scalar*) inElems[1])->getFloat() + 1);
                outElems.push_back(&e2);
                ParamList paramList(outElems);
                CallResponse callResponse1(callRequest->getMessageHeader().getMessageId(),
                        Status::SUCCESS);
                callResponse1.setMethodId(&methodId);
                callResponse1.setParamId(&paramId);
                callResponse1.setParamList(&paramList);
                sendMessage(serverSocket, &callResponse1);
            }
        }

        static void* callErrorsThreadRun(void* object) {
            ServerSocket& serverSocket = *(ServerSocket*) object;
            // send a response with an error code
            Call& callRequest = *static_cast<Call*> (readMessage(serverSocket));
            ScopeGuard<Call> callRequestSG(&callRequest);
            CallResponse callResponse(callRequest.getMessageHeader().getMessageId(),
                    Status::INVALID_MESSAGE);
            sendMessage(serverSocket, &callResponse);
        }

        static void* subscribeThreadRun(void* object) {
            ServerSocket& serverSocket = *(ServerSocket*) object;
            // send response for subscribe request 1 (ns=1;i=11)
            Subscribe& subscribeRequest1 = *static_cast<Subscribe*> (readMessage(serverSocket));
            ScopeGuard<Subscribe> subscribeRequest1SG(&subscribeRequest1);
            SubscribeResponse subscribeResponse1(subscribeRequest1.getMessageHeader().getMessageId(),
                    Status::SUCCESS);
            sendMessage(serverSocket, &subscribeResponse1);
            // send response for subscribe request 2 (eventTypeId ns=1;i=44)
            Subscribe& subscribeRequest2 = *static_cast<Subscribe*> (readMessage(serverSocket));
            ScopeGuard<Subscribe> subscribeRequest2SG(&subscribeRequest2);
            SubscribeResponse subscribeResponse2(subscribeRequest2.getMessageHeader().getMessageId(),
                    Status::SUCCESS);
            sendMessage(serverSocket, &subscribeResponse2);
            // send a notification
            std::map<const ParamId*, const Variant*> elements;
            const ParamId& paramId = subscribeRequest1.getParamId();
            Scalar paramValue;
            paramValue.setInt(112);
            elements[&paramId] = &paramValue;
            Notification notification(3 /*messageId*/, *new ParamMap(elements),
                    true /*attachValues*/);
            sendMessage(serverSocket, &notification);
            // send an event
            Event event(4 /*messageId*/, *new ParamId(subscribeRequest1.getParamId()) /*eventId*/,
                    *new ParamId(55) /*paramId*/, time(NULL) * 1000, 500 /*severity*/,
                    *new std::string("huhu") /*message*/,
                    *new ParamMap(elements), true /*attachValues*/);
            sendMessage(serverSocket, &event);
            // send response for read request 1
            Read& readRequest1 = *static_cast<Read*> (readMessage(serverSocket));
            ScopeGuard<Read> readRequest1SG(&readRequest1);
            ReadResponse readResponse1(readRequest1.getMessageHeader().getMessageId(),
                    Status::SUCCESS);
            readResponse1.setParamId(&readRequest1.getParamId());
            Scalar paramValue1;
            paramValue1.setInt(111);
            readResponse1.setParamValue(&paramValue1);
            sendMessage(serverSocket, &readResponse1);
            // send response for read request 2
            Read& readRequest2 = *static_cast<Read*> (readMessage(serverSocket));
            ScopeGuard<Read> readRequest2SG(&readRequest2);
            ReadResponse readResponse2(readRequest2.getMessageHeader().getMessageId(),
                    Status::SUCCESS);
            readResponse2.setParamId(&readRequest2.getParamId());
            Scalar paramValue2;
            paramValue2.setInt(441);
            readResponse2.setParamValue(&paramValue2);
            sendMessage(serverSocket, &readResponse2);
            // send response for unsubscribe request 2 (eventTypeId ns=1;i=44)
            Unsubscribe& unsubscribeRequest2 = *static_cast<Unsubscribe*> (readMessage(serverSocket));
            ScopeGuard<Unsubscribe> unsubscribeRequest2SG(&unsubscribeRequest2);
            UnsubscribeResponse unsubscribeResponse2(unsubscribeRequest2.getMessageHeader().getMessageId(),
                    Status::SUCCESS);
            sendMessage(serverSocket, &unsubscribeResponse2);
            // send response for unsubscribe request 1 (ns=1;i=11)
            Unsubscribe& unsubscribeRequest1 = *static_cast<Unsubscribe*> (readMessage(serverSocket));
            ScopeGuard<Unsubscribe> unsubscribeRequest1SG(&unsubscribeRequest1);
            UnsubscribeResponse unsubscribeResponse1(unsubscribeRequest1.getMessageHeader().getMessageId(),
                    Status::SUCCESS);
            sendMessage(serverSocket, &unsubscribeResponse1);
        }

        static void* subscribeErrorsThreadRun(void* object) {
            ServerSocket& serverSocket = *(ServerSocket*) object;
            // send a subscribe response with an error code
            Subscribe& subscribeRequest = *static_cast<Subscribe*> (readMessage(serverSocket));
            ScopeGuard<Subscribe> subscribeRequestSG(&subscribeRequest);
            SubscribeResponse subscribeResponse(subscribeRequest.getMessageHeader().getMessageId(),
                    Status::INVALID_MESSAGE);
            sendMessage(serverSocket, &subscribeResponse);
            // send an unsubscribe response with an error code
            Unsubscribe& unsubscribeRequest = *static_cast<Unsubscribe*> (readMessage(serverSocket));
            ScopeGuard<Unsubscribe> unsubscribeRequestSG(&unsubscribeRequest);
            UnsubscribeResponse unsubscribeResponse(unsubscribeRequest.getMessageHeader().getMessageId(),
                    Status::INVALID_MESSAGE);
            sendMessage(serverSocket, &unsubscribeResponse);
        }

        static void* renewSubscriptionInitThreadRun(void* object) {
            ServerSocket& serverSocket = *(ServerSocket*) object;
            // send response for subscribe request 1 (ns=1;i=11)
            Subscribe& subscribeRequest1 = *static_cast<Subscribe*> (readMessage(serverSocket));
            ScopeGuard<Subscribe> subscribeRequest1SG(&subscribeRequest1);
            CHECK_EQUAL(11, subscribeRequest1.getParamId().getNumeric());
            SubscribeResponse subscribeResponse1(subscribeRequest1.getMessageHeader().getMessageId(),
                    Status::SUCCESS);
            sendMessage(serverSocket, &subscribeResponse1);
            // send response for subscribe request 2 (ns=1;i=44)
            Subscribe& subscribeRequest2 = *static_cast<Subscribe*> (readMessage(serverSocket));
            ScopeGuard<Subscribe> subscribeRequest2SG(&subscribeRequest2);
            CHECK_EQUAL(44, subscribeRequest2.getParamId().getNumeric());
            SubscribeResponse subscribeResponse2(subscribeRequest2.getMessageHeader().getMessageId(),
                    Status::SUCCESS);
            sendMessage(serverSocket, &subscribeResponse2);

            // send response for read request 1
            Read& readRequest1 = *static_cast<Read*> (readMessage(serverSocket));
            ScopeGuard<Read> readRequest1SG(&readRequest1);
            CHECK_EQUAL(11, readRequest1.getParamId().getNumeric());
            ReadResponse readResponse1(readRequest1.getMessageHeader().getMessageId(),
                    Status::SUCCESS);
            readResponse1.setParamId(&readRequest1.getParamId());
            Scalar paramValue1;
            paramValue1.setInt(111);
            readResponse1.setParamValue(&paramValue1);
            sendMessage(serverSocket, &readResponse1);
            // send response for read request 2
            Read& readRequest2 = *static_cast<Read*> (readMessage(serverSocket));
            ScopeGuard<Read> readRequest2SG(&readRequest2);
            CHECK_EQUAL(44, readRequest2.getParamId().getNumeric());
            ReadResponse readResponse2(readRequest2.getMessageHeader().getMessageId(),
                    Status::SUCCESS);
            readResponse2.setParamId(&readRequest2.getParamId());
            Scalar paramValue2;
            paramValue2.setInt(441);
            readResponse2.setParamValue(&paramValue2);
            sendMessage(serverSocket, &readResponse2);
        }

        static void* renewSubscriptionThreadRun(void* object) {
            ServerSocket& serverSocket = *(ServerSocket*) object;

            // send response for unsubscribe request 2 (ns=1;i=44)
            Unsubscribe& unsubscribeRequest2 = *static_cast<Unsubscribe*> (readMessage(serverSocket));
            ScopeGuard<Unsubscribe> unsubscribeRequest2SG(&unsubscribeRequest2);
            CHECK_EQUAL(44, unsubscribeRequest2.getParamId().getNumeric());
            UnsubscribeResponse unsubscribeResponse2(unsubscribeRequest2.getMessageHeader().getMessageId(),
                    Status::SUCCESS);
            sendMessage(serverSocket, &unsubscribeResponse2);
            // send response for subscribe request 2 (ns=1;i=44)
            Subscribe& subscribeRequest1 = *static_cast<Subscribe*> (readMessage(serverSocket));
            ScopeGuard<Subscribe> subscribeRequest1SG(&subscribeRequest1);
            CHECK_EQUAL(44, subscribeRequest1.getParamId().getNumeric());
            SubscribeResponse subscribeResponse1(subscribeRequest1.getMessageHeader().getMessageId(),
                    Status::SUCCESS);
            sendMessage(serverSocket, &subscribeResponse1);

            // send response for unsubscribe request 1 (ns=1;i=11)
            Unsubscribe& unsubscribeRequest1 = *static_cast<Unsubscribe*> (readMessage(serverSocket));
            ScopeGuard<Unsubscribe> unsubscribeRequest1SG(&unsubscribeRequest1);
            CHECK_EQUAL(11, unsubscribeRequest1.getParamId().getNumeric());
            UnsubscribeResponse unsubscribeResponse1(unsubscribeRequest1.getMessageHeader().getMessageId(),
                    Status::SUCCESS);
            sendMessage(serverSocket, &unsubscribeResponse1);
            // send response for subscribe request 1 (ns=1;i=11)
            Subscribe& subscribeRequest2 = *static_cast<Subscribe*> (readMessage(serverSocket));
            ScopeGuard<Subscribe> subscribeRequest2SG(&subscribeRequest2);
            CHECK_EQUAL(11, subscribeRequest2.getParamId().getNumeric());
            SubscribeResponse subscribeResponse2(subscribeRequest2.getMessageHeader().getMessageId(),
                    Status::SUCCESS);
            sendMessage(serverSocket, &subscribeResponse2);

            // send response for unsubscribe request 2 (ns=1;i=44)
            Unsubscribe& unsubscribeRequest4 = *static_cast<Unsubscribe*> (readMessage(serverSocket));
            ScopeGuard<Unsubscribe> unsubscribeRequest4SG(&unsubscribeRequest4);
            CHECK_EQUAL(44, unsubscribeRequest4.getParamId().getNumeric());
            UnsubscribeResponse unsubscribeResponse4(unsubscribeRequest4.getMessageHeader().getMessageId(),
                    Status::SUCCESS);
            sendMessage(serverSocket, &unsubscribeResponse4);
            // send response for unsubscribe request 1 (ns=1;i=11)
            Unsubscribe& unsubscribeRequest3 = *static_cast<Unsubscribe*> (readMessage(serverSocket));
            ScopeGuard<Unsubscribe> unsubscribeRequest3SG(&unsubscribeRequest3);
            CHECK_EQUAL(11, unsubscribeRequest3.getParamId().getNumeric());
            UnsubscribeResponse unsubscribeResponse3(unsubscribeRequest3.getMessageHeader().getMessageId(),
                    Status::SUCCESS);
            sendMessage(serverSocket, &unsubscribeResponse3);
        }

        static void* renewSubscriptionErrorsThreadRun1(void* object) {
            ServerSocket& serverSocket = *(ServerSocket*) object;

            // send response for unsubscribe request 2 (ns=1;i=44)
            Unsubscribe& unsubscribeRequest2 = *static_cast<Unsubscribe*> (readMessage(serverSocket));
            ScopeGuard<Unsubscribe> unsubscribeRequest2SG(&unsubscribeRequest2);
            CHECK_EQUAL(44, unsubscribeRequest2.getParamId().getNumeric());
            UnsubscribeResponse unsubscribeResponse2(unsubscribeRequest2.getMessageHeader().getMessageId(),
                    Status::INVALID_MESSAGE);
            sendMessage(serverSocket, &unsubscribeResponse2);
            // a response for subscribe request 2 need not be send because unsubscribing failed           

            // send response for unsubscribe request 1 (ns=1;i=11)
            Unsubscribe& unsubscribeRequest1 = *static_cast<Unsubscribe*> (readMessage(serverSocket));
            ScopeGuard<Unsubscribe> unsubscribeRequest1SG(&unsubscribeRequest1);
            CHECK_EQUAL(11, unsubscribeRequest1.getParamId().getNumeric());
            UnsubscribeResponse unsubscribeResponse1(unsubscribeRequest1.getMessageHeader().getMessageId(),
                    Status::SUCCESS);
            sendMessage(serverSocket, &unsubscribeResponse1);
            // send response for subscribe request 1 (ns=1;i=11)
            Subscribe& subscribeRequest1 = *static_cast<Subscribe*> (readMessage(serverSocket));
            ScopeGuard<Subscribe> subscribeRequest1SG(&subscribeRequest1);
            CHECK_EQUAL(11, subscribeRequest1.getParamId().getNumeric());
            SubscribeResponse subscribeResponse1(subscribeRequest1.getMessageHeader().getMessageId(),
                    Status::SUCCESS);
            sendMessage(serverSocket, &subscribeResponse1);
        }

        static void* renewSubscriptionErrorsThreadRun2(void* object) {
            ServerSocket& serverSocket = *(ServerSocket*) object;

            // send response for unsubscribe request 2 (ns=1;i=44)
            Unsubscribe& unsubscribeRequest2 = *static_cast<Unsubscribe*> (readMessage(serverSocket));
            ScopeGuard<Unsubscribe> unsubscribeRequest2SG(&unsubscribeRequest2);
            CHECK_EQUAL(44, unsubscribeRequest2.getParamId().getNumeric());
            UnsubscribeResponse unsubscribeResponse2(unsubscribeRequest2.getMessageHeader().getMessageId(),
                    Status::SUCCESS);
            sendMessage(serverSocket, &unsubscribeResponse2);
            // send response for subscribe request 2 (ns=1;i=44)
            Subscribe& subscribeRequest2 = *static_cast<Subscribe*> (readMessage(serverSocket));
            ScopeGuard<Subscribe> subscribeRequest2SG(&subscribeRequest2);
            CHECK_EQUAL(44, subscribeRequest2.getParamId().getNumeric());
            SubscribeResponse subscribeResponse2(subscribeRequest2.getMessageHeader().getMessageId(),
                    Status::INVALID_MESSAGE);
            sendMessage(serverSocket, &subscribeResponse2);

            // send response for unsubscribe request 1 (ns=1;i=11)
            Unsubscribe& unsubscribeRequest1 = *static_cast<Unsubscribe*> (readMessage(serverSocket));
            ScopeGuard<Unsubscribe> unsubscribeRequest1SG(&unsubscribeRequest1);
            CHECK_EQUAL(11, unsubscribeRequest1.getParamId().getNumeric());
            UnsubscribeResponse unsubscribeResponse1(unsubscribeRequest1.getMessageHeader().getMessageId(),
                    Status::SUCCESS);
            sendMessage(serverSocket, &unsubscribeResponse1);
            // send response for subscribe request 1 (ns=1;i=11)
            Subscribe& subscribeRequest1 = *static_cast<Subscribe*> (readMessage(serverSocket));
            ScopeGuard<Subscribe> subscribeRequest1SG(&subscribeRequest1);
            CHECK_EQUAL(11, subscribeRequest1.getParamId().getNumeric());
            SubscribeResponse subscribeResponse1(subscribeRequest1.getMessageHeader().getMessageId(),
                    Status::SUCCESS);
            sendMessage(serverSocket, &subscribeResponse1);

            // send response for unsubscribe request 2 (ns=1;i=44)
            Unsubscribe& unsubscribeRequest4 = *static_cast<Unsubscribe*> (readMessage(serverSocket));
            ScopeGuard<Unsubscribe> unsubscribeRequest4SG(&unsubscribeRequest4);
            CHECK_EQUAL(44, unsubscribeRequest4.getParamId().getNumeric());
            UnsubscribeResponse unsubscribeResponse4(unsubscribeRequest4.getMessageHeader().getMessageId(),
                    Status::SUCCESS);
            sendMessage(serverSocket, &unsubscribeResponse4);
            // send response for unsubscribe request 1 (ns=1;i=11)
            Unsubscribe& unsubscribeRequest3 = *static_cast<Unsubscribe*> (readMessage(serverSocket));
            ScopeGuard<Unsubscribe> unsubscribeRequest3SG(&unsubscribeRequest3);
            CHECK_EQUAL(11, unsubscribeRequest3.getParamId().getNumeric());
            UnsubscribeResponse unsubscribeResponse3(unsubscribeRequest3.getMessageHeader().getMessageId(),
                    Status::SUCCESS);
            sendMessage(serverSocket, &unsubscribeResponse3);
        }
    };

    TEST(ProviderBinaryIoDataProvider_BinaryIODataProvider, OpenClose) {
        // open a server socket
        ServerSocket serverSocket(10 /*acceptTimeout*/, 10 /*sendReceiveTimeout*/,
                true /*reuseAddress*/);
        serverSocket.open(Env::PORT1); // see config for port
        // start a server thread
        pthread_t serverThread;
        pthread_create(&serverThread, NULL, &openCloseThreadRun, &serverSocket);
        // wait a second
        timespec delay;
        delay.tv_sec = 1;
        delay.tv_nsec = 0;
        nanosleep(&delay, NULL /* remaining */);
        // open the data provider
        Env env;
        std::string confDir = env.getApplicationDir() + "/conf/provider/binary/";
        BinaryIODataProvider provider(true /*unitTest*/);
        provider.open(confDir);
        // close the data provider and wait for the end of the server thread
        provider.close();
        pthread_join(serverThread, NULL /*return*/);
        // close the server socket
        serverSocket.close();
    }

    TEST(ProviderBinaryIoDataProvider_BinaryIODataProvider, OpenErrors) {
        // try to open the data provider with a missing config file
        Env env;
        std::string confDir = env.getApplicationDir() + "/conf/provider/binary/XXXXX";
        BinaryIODataProvider provider(true /*unitTest*/);
        try {
            provider.open(confDir);
            FAIL("");
        } catch (IODataProviderNamespace::IODataProviderException& e) {
            STRCMP_CONTAINS("Cannot open IO data provider using configuration",
                    e.getMessage().c_str());
            STRCMP_CONTAINS("Cannot parse configuration file", e.getCause()->getMessage().c_str());
        }

        // try to open the data provider without an existing server part
        confDir = env.getApplicationDir() + "/conf/provider/binary/";
        try {
            provider.open(confDir);
            FAIL("");
        } catch (IODataProviderNamespace::IODataProviderException& e) {
            // the connectTimeout has been reached
            STRCMP_CONTAINS("Cannot open IO data provider using configuration",
                    e.getMessage().c_str());
            STRCMP_CONTAINS("Time out after 2 sec.", e.getCause()->getMessage().c_str());
        }
    }

    TEST(ProviderBinaryIoDataProvider_BinaryIODataProvider, Read) {
        // open a server socket
        ServerSocket serverSocket(10 /*acceptTimeout*/, 10 /*sendReceiveTimeout*/,
                true /*reuseAddress*/);
        serverSocket.open(Env::PORT1); // see config for port
        // start a server thread
        pthread_t serverThread;
        pthread_create(&serverThread, NULL, &readThreadRun, &serverSocket);
        timespec delay;
        delay.tv_sec = 1;
        delay.tv_nsec = 0;
        nanosleep(&delay, NULL /* remaining */);
        // open the data provider
        Env env;
        std::string confDir = env.getApplicationDir() + "/conf/provider/binary/";
        BinaryIODataProvider provider(true /*unitTest*/);
        provider.open(confDir);
        // the binary interface does not support namespaces 
        // => get default node properties with namespace index before calling a method
        int nsIndex = 1;
        const IODataProviderNamespace::NodeProperties* dfltNodeProps =
                provider.getDefaultNodeProperties(std::string("ns"), nsIndex);
        delete dfltNodeProps;
        // read values
        std::vector<const IODataProviderNamespace::NodeId*> nodeIds;
        // 1. request (boolean)
        IODataProviderNamespace::NodeId n1(nsIndex, 11 /*id*/);
        nodeIds.push_back(&n1);
        // 2. request (char)
        IODataProviderNamespace::NodeId n2(nsIndex, 12 /*id*/);
        nodeIds.push_back(&n2);
        // 3. request (byte -> schar)
        IODataProviderNamespace::NodeId n3(nsIndex, 13 /*id*/);
        nodeIds.push_back(&n3);
        // 4. request (short -> int)
        IODataProviderNamespace::NodeId n4(nsIndex, 14 /*id*/);
        nodeIds.push_back(&n4);
        // 5. request (int -> long)
        IODataProviderNamespace::NodeId n5(nsIndex, 15 /*id*/);
        nodeIds.push_back(&n5);
        // 6. request (long -> llong)
        IODataProviderNamespace::NodeId n6(nsIndex, 16 /*id*/);
        nodeIds.push_back(&n6);
        // 7. request (float)
        IODataProviderNamespace::NodeId n7(nsIndex, 17 /*id*/);
        nodeIds.push_back(&n7);
        // 8. request (double)
        IODataProviderNamespace::NodeId n8(nsIndex, 18 /*id*/);
        nodeIds.push_back(&n8);
        // 9. request (structure)
        IODataProviderNamespace::NodeId n9(nsIndex, 19 /*id*/);
        nodeIds.push_back(&n9);
        // 10. request (char[] -> string)
        std::string id("20");
        IODataProviderNamespace::NodeId n10(nsIndex, id);
        nodeIds.push_back(&n10);
        // 11. request (byte[] -> byteString)
        IODataProviderNamespace::NodeId n11(nsIndex, 21 /*id*/);
        nodeIds.push_back(&n11);
        // 12. request (structure[])
        IODataProviderNamespace::NodeId n12(nsIndex, 22 /*id*/);
        nodeIds.push_back(&n12);
        // 13. request (char[][] -> string[])
        IODataProviderNamespace::NodeId n13(nsIndex, 23 /*id*/);
        nodeIds.push_back(&n13);
        std::vector<IODataProviderNamespace::NodeData*>* result = provider.read(nodeIds);
        VectorScopeGuard<IODataProviderNamespace::NodeData> resultSG(result);
        int requestNo = 1;
        for (std::vector<IODataProviderNamespace::NodeData*>::iterator it = result->begin();
                it != result->end(); it++) {
            IODataProviderNamespace::NodeData& nodeData = **it;
            CHECK_EQUAL(nsIndex, nodeData.getNodeId().getNamespaceIndex());
            switch (requestNo) {
                case 9: // structure
                {
                    CHECK_EQUAL(requestNo + 10, nodeData.getNodeId().getNumeric());
                    const IODataProviderNamespace::Structure& structValue =
                            *static_cast<const IODataProviderNamespace::Structure*> (nodeData.getData());
                    CHECK_EQUAL(nsIndex, structValue.getDataTypeId().getNamespaceIndex());
                    CHECK_EQUAL(110, structValue.getDataTypeId().getNumeric());
                    CHECK_EQUAL(2, structValue.getFieldData().size());

                    const IODataProviderNamespace::Scalar& fieldValue1 =
                            *static_cast<const IODataProviderNamespace::Scalar*> (
                            structValue.getFieldData().at(std::string("x1")));
                    LONGS_EQUAL(1111, fieldValue1.getLong());

                    const IODataProviderNamespace::Scalar& fieldValue2 =
                            *static_cast<const IODataProviderNamespace::Scalar*> (
                            structValue.getFieldData().at(std::string("x2")));
                    LONGS_EQUAL(1112, fieldValue2.getLong());
                    break;
                }
                case 10: // char[] -> string 
                {
                    STRCMP_EQUAL("20", nodeData.getNodeId().getString().c_str());
                    const IODataProviderNamespace::Scalar& paramValue =
                            *static_cast<const IODataProviderNamespace::Scalar*> (nodeData.getData());
                    STRCMP_EQUAL("ab", paramValue.getString()->c_str());
                    break;
                }
                case 12: // structure[]
                {
                    CHECK_EQUAL(requestNo + 10, nodeData.getNodeId().getNumeric());
                    const IODataProviderNamespace::Array& arrayValue =
                            *static_cast<const IODataProviderNamespace::Array*> (nodeData.getData());
                    CHECK_EQUAL(2, arrayValue.getElements()->size());

                    const IODataProviderNamespace::Structure& structValue1 =
                            *static_cast<const IODataProviderNamespace::Structure*> (
                            arrayValue.getElements()->at(0));
                    CHECK_EQUAL(nsIndex, structValue1.getDataTypeId().getNamespaceIndex());
                    CHECK_EQUAL(1111, structValue1.getDataTypeId().getNumeric());
                    CHECK_EQUAL(1, structValue1.getFieldData().size());
                    const IODataProviderNamespace::Scalar& fieldValue1 =
                            *static_cast<const IODataProviderNamespace::Scalar*> (
                            structValue1.getFieldData().at(std::string("x1")));
                    LONGS_EQUAL(1121, fieldValue1.getLong());

                    const IODataProviderNamespace::Structure& structValue2 =
                            *static_cast<const IODataProviderNamespace::Structure*> (
                            arrayValue.getElements()->at(1));
                    CHECK_EQUAL(nsIndex, structValue2.getDataTypeId().getNamespaceIndex());
                    CHECK_EQUAL(1112, structValue2.getDataTypeId().getNumeric());
                    CHECK_EQUAL(1, structValue2.getFieldData().size());
                    const IODataProviderNamespace::Scalar& fieldValue2 =
                            *static_cast<const IODataProviderNamespace::Scalar*> (
                            structValue2.getFieldData().at(std::string("x2")));
                    LONGS_EQUAL(1122, fieldValue2.getLong());
                    break;
                }
                case 13: // char[][] -> string[]
                {
                    CHECK_EQUAL(requestNo + 10, nodeData.getNodeId().getNumeric());
                    const IODataProviderNamespace::Array& arrayValue =
                            *static_cast<const IODataProviderNamespace::Array*> (nodeData.getData());
                    CHECK_EQUAL(2, arrayValue.getElements()->size());
                    CHECK_EQUAL(IODataProviderNamespace::Scalar::STRING, arrayValue.getArrayType());
                    const IODataProviderNamespace::Scalar* paramValue =
                            static_cast<const IODataProviderNamespace::Scalar*> (arrayValue.getElements()->at(0));
                    STRCMP_EQUAL("ab", paramValue->getString()->c_str());
                    paramValue = static_cast<const IODataProviderNamespace::Scalar*> (arrayValue.getElements()->at(1));
                    STRCMP_EQUAL("cd", paramValue->getString()->c_str());
                }
                default:
                {
                    CHECK_EQUAL(requestNo + 10, nodeData.getNodeId().getNumeric());
                    const IODataProviderNamespace::Scalar& paramValue =
                            *static_cast<const IODataProviderNamespace::Scalar*> (nodeData.getData());
                    switch (requestNo) {
                        case 1: // boolean
                            CHECK_TRUE(paramValue.getBool());
                            break;
                        case 2: // char
                            CHECK_EQUAL('a', paramValue.getChar());
                            break;
                        case 3: // byte -> schar
                            CHECK_EQUAL(static_cast<signed char> (33), paramValue.getSChar());
                            break;
                        case 4: // short -> int
                            CHECK_EQUAL(44, paramValue.getInt());
                            break;
                        case 5: // int -> long
                            CHECK_EQUAL(55, paramValue.getLong());
                            break;
                        case 6: // long -> llong
                            LONGS_EQUAL(66, paramValue.getLLong());
                            break;
                        case 7: // float
                            DOUBLES_EQUAL(7.7, paramValue.getFloat(), 0.01);
                            break;
                        case 8: // double
                            DOUBLES_EQUAL(8.8, paramValue.getDouble(), 0.01);
                            break;
                        case 11: // byte[] -> byteString
                        {
                            CHECK_EQUAL(2, paramValue.getByteStringLength());
                            const char* bs = paramValue.getByteString();
                            CHECK_EQUAL(34, bs[0] & 0xFF);
                            CHECK_EQUAL(255, bs[1] & 0xFF);
                            break;
                        }
                    }
                    break;
                }
            }
            requestNo++;
        }
        // do not call "close" to provider and server socket
        // because the destructors do it internally
        // only wait for end of server thread
        pthread_join(serverThread, NULL /*return*/);
    }

    TEST(ProviderBinaryIoDataProvider_BinaryIODataProvider, ReadErrors) {
        /// open a server socket
        ServerSocket serverSocket(3 /*acceptTimeout*/, 10 /*sendReceiveTimeout*/,
                true /*reuseAddress*/);
        serverSocket.open(Env::PORT1); // see config for port

        // open the data provider
        Env env;
        std::string confDir = env.getApplicationDir() + "/conf/provider/binary/";
        BinaryIODataProvider provider(true /*unitTest*/);
        provider.open(confDir);
        // close the server socket
        serverSocket.close();
        // wait some time: 
        // the provider tries to reconnect after 1 sec. and 3 sec. 
        // (see maxReconnectDelay in config.xml), after 5s the reconnect succeeds
        timespec delay;
        delay.tv_sec = 4;
        delay.tv_nsec = 0;
        nanosleep(&delay, NULL /* remaining */);
        serverSocket.open(Env::PORT1);

        // try to read value of a node 
        // (the provider is not connected yet and the server does not accept connections)
        std::vector<const IODataProviderNamespace::NodeId*> nodeIds;
        IODataProviderNamespace::NodeId n1(1 /*nsIndex*/, 2 /*id*/);
        nodeIds.push_back(&n1);
        std::vector<IODataProviderNamespace::NodeData*>* results = provider.read(nodeIds);
        VectorScopeGuard<IODataProviderNamespace::NodeData> resultsSG(results);
        CHECK_EQUAL(1, results->size());
        IODataProviderNamespace::IODataProviderException* ex = (*results)[0]->getException();
        // the request cannot be sent
        STRCMP_CONTAINS("Cannot read data for ", ex->getMessage().c_str());
        STRCMP_CONTAINS("Cannot write data due to a closed socket",
                ex->getCause()->getMessage().c_str());
        // the client is connected => close client socket of the provider
        provider.close();

        // reopen the server socket 
        // (the connection is still active and would be accepted by the server;
        // the reading from the connection would fail with "Connection closed by remote side")
        serverSocket.close();
        serverSocket.open(Env::PORT1); // see config for port

        // start a server thread
        pthread_t serverThread;
        pthread_create(&serverThread, NULL, &readErrorsThreadRun, &serverSocket);
        delay.tv_sec = 1;
        nanosleep(&delay, NULL /* remaining */);
        // open the data provider
        provider.open(confDir);
        // try to read value of a node (the server sends a response with an error)        
        results = provider.read(nodeIds);
        VectorScopeGuard<IODataProviderNamespace::NodeData> resultsSG2(results);
        CHECK_EQUAL(1, results->size());
        ex = (*results)[0]->getException();
        STRCMP_CONTAINS("Received a read response for", ex->getMessage().c_str());
        // close the provider and wait for end of server thread
        provider.close();
        pthread_join(serverThread, NULL /*return*/);

        // close the server socket
        serverSocket.close();
    }

    TEST(ProviderBinaryIoDataProvider_BinaryIODataProvider, Write) {
        /// open a server socket
        ServerSocket serverSocket(10 /*acceptTimeout*/, 10 /*sendReceiveTimeout*/,
                true /*reuseAddress*/);
        serverSocket.open(Env::PORT1); // see config for port
        // start a server thread
        pthread_t serverThread;
        pthread_create(&serverThread, NULL, &writeThreadRun, &serverSocket);
        timespec delay;
        delay.tv_sec = 1;
        delay.tv_nsec = 0;
        nanosleep(&delay, NULL /* remaining */);
        // open the data provider
        Env env;
        std::string confDir = env.getApplicationDir() + "/conf/provider/binary/";
        BinaryIODataProvider provider(true /*unitTest*/);
        provider.open(confDir);
        // write values
        std::vector<const IODataProviderNamespace::NodeData*> nodeData;
        // bool
        std::string id("11");
        IODataProviderNamespace::Scalar* s = new IODataProviderNamespace::Scalar();
        s->setBool(true);
        IODataProviderNamespace::NodeData n1(*new IODataProviderNamespace::NodeId(1, id),
                s, true /* attachValues*/);
        nodeData.push_back(&n1);
        // byte
        s = new IODataProviderNamespace::Scalar();
        s->setSChar(0x22);
        IODataProviderNamespace::NodeData n2(*new IODataProviderNamespace::NodeId(1, 12 /*id*/),
                s, true /* attachValues*/);
        nodeData.push_back(&n2);
        // char
        s = new IODataProviderNamespace::Scalar();
        s->setChar('a');
        IODataProviderNamespace::NodeData n3(*new IODataProviderNamespace::NodeId(1, 13 /*id*/),
                s, true /* attachValues*/);
        nodeData.push_back(&n3);
        // int
        s = new IODataProviderNamespace::Scalar();
        s->setInt(44);
        IODataProviderNamespace::NodeData n4(*new IODataProviderNamespace::NodeId(1, 14 /*id*/),
                s, true /* attachValues*/);
        nodeData.push_back(&n4);
        // long
        s = new IODataProviderNamespace::Scalar();
        s->setLong(55);
        IODataProviderNamespace::NodeData n5(*new IODataProviderNamespace::NodeId(1, 15 /*id*/),
                s, true /* attachValues*/);
        nodeData.push_back(&n5);
        // llong
        s = new IODataProviderNamespace::Scalar();
        s->setLLong(66);
        IODataProviderNamespace::NodeData n6(*new IODataProviderNamespace::NodeId(1, 16 /*id*/),
                s, true /* attachValues*/);
        nodeData.push_back(&n6);
        // float
        s = new IODataProviderNamespace::Scalar();
        s->setFloat(7.7);
        IODataProviderNamespace::NodeData n7(*new IODataProviderNamespace::NodeId(1, 17 /*id*/),
                s, true /* attachValues*/);
        nodeData.push_back(&n7);
        // double
        s = new IODataProviderNamespace::Scalar();
        s->setDouble(8.8);
        IODataProviderNamespace::NodeData n8(*new IODataProviderNamespace::NodeId(1, 18 /*id*/),
                s, true /* attachValues*/);
        nodeData.push_back(&n8);
        // string
        s = new IODataProviderNamespace::Scalar();
        s->setString(new std::string("ab"), true /*attachValues*/);
        IODataProviderNamespace::NodeData n9(*new IODataProviderNamespace::NodeId(1, 19 /*id*/),
                s, true /* attachValues*/);
        nodeData.push_back(&n9);

        s = new IODataProviderNamespace::Scalar();
        s->setString(NULL, true /*attachValues*/);
        IODataProviderNamespace::NodeData n91(*new IODataProviderNamespace::NodeId(1, 191 /*id*/),
                s, true /* attachValues*/);
        nodeData.push_back(&n91);
        // structure
        IODataProviderNamespace::NodeId structId10(1, 110);

        std::map<std::string, const IODataProviderNamespace::Variant*> fields10;
        IODataProviderNamespace::Scalar fieldValue101;
        fieldValue101.setInt(1111);
        fields10[std::string("x1")] = &fieldValue101;

        IODataProviderNamespace::Scalar fieldValue102;
        fieldValue102.setInt(1112);
        fields10[std::string("x2")] = &fieldValue102;

        IODataProviderNamespace::Structure* struc10 = new IODataProviderNamespace::Structure(
                structId10, fields10);
        IODataProviderNamespace::NodeData n10(*new IODataProviderNamespace::NodeId(1, 20 /*id*/),
                struc10, true /* attachValues*/);
        nodeData.push_back(&n10);
        // array of structure
        IODataProviderNamespace::NodeId structId111(1, 1111 /*id*/);
        std::map<std::string, const IODataProviderNamespace::Variant*> fields111;
        IODataProviderNamespace::Scalar fieldValue111;
        fieldValue111.setInt(1121);
        fields111[std::string("x1")] = &fieldValue111;
        IODataProviderNamespace::Structure struc111(structId111, fields111);

        IODataProviderNamespace::NodeId structId112(1, 1112 /*id*/);
        std::map<std::string, const IODataProviderNamespace::Variant*> fields112;
        IODataProviderNamespace::Scalar fieldValue112;
        fieldValue112.setInt(1122);
        fields112[std::string("x2")] = &fieldValue112;
        IODataProviderNamespace::Structure struc112(structId112, fields112);

        std::vector<const IODataProviderNamespace::Variant*> arrayElements11;
        arrayElements11.push_back(&struc111);
        arrayElements11.push_back(&struc112);
        IODataProviderNamespace::Array* array11 = new IODataProviderNamespace::Array(
                IODataProviderNamespace::Variant::STRUCTURE, &arrayElements11);

        IODataProviderNamespace::NodeData n11(*new IODataProviderNamespace::NodeId(1, 21 /*id*/),
                array11, true /* attachValues*/);
        nodeData.push_back(&n11);
        // byte string
        s = new IODataProviderNamespace::Scalar();
        char* bs = new char[2];
        bs[0] = 34;
        bs[1] = 255;
        s->setByteString(bs, 2 /* length*/, true /*attachValues*/);
        IODataProviderNamespace::NodeData n12(*new IODataProviderNamespace::NodeId(1, 22 /*id*/),
                s, true /* attachValues*/);
        nodeData.push_back(&n12);

        s = new IODataProviderNamespace::Scalar();
        s->setByteString(NULL, -1 /* length*/, true /*attachValues*/);
        IODataProviderNamespace::NodeData n13(*new IODataProviderNamespace::NodeId(1, 23 /*id*/),
                s, true /* attachValues*/);
        nodeData.push_back(&n13);
        // localizedText -> char[]
        s = new IODataProviderNamespace::Scalar();
        s->setLocalizedText(new std::string("en"), new std::string("ab"), true /*attachValues*/);
        IODataProviderNamespace::NodeData n14(*new IODataProviderNamespace::NodeId(1, 24 /*id*/),
                s, true /* attachValues*/);
        nodeData.push_back(&n14);

        s = new IODataProviderNamespace::Scalar();
        s->setLocalizedText(NULL, NULL, true /*attachValues*/);
        IODataProviderNamespace::NodeData n15(*new IODataProviderNamespace::NodeId(1, 25 /*id*/),
                s, true /* attachValues*/);
        nodeData.push_back(&n15);

        // string[] -> char[][]
        std::vector<const IODataProviderNamespace::Variant*>* arrayElements
                = new std::vector<const IODataProviderNamespace::Variant*>();
        s = new IODataProviderNamespace::Scalar();
        s->setString(new std::string("ab"), true /*attachValues*/);
        arrayElements->push_back(s);
        s = new IODataProviderNamespace::Scalar();
        s->setString(new std::string("cd"), true /*attachValues*/);
        arrayElements->push_back(s);
        IODataProviderNamespace::Array* array = new IODataProviderNamespace::Array(
                IODataProviderNamespace::Scalar::STRING, arrayElements, true /* attachValues */);
        IODataProviderNamespace::NodeData n16(*new IODataProviderNamespace::NodeId(1, 26 /*id*/),
                array, true /* attachValues*/);
        nodeData.push_back(&n16);

        // byteString[] -> byte[][]
        std::vector<const IODataProviderNamespace::Variant*> arrayElements27;
        IODataProviderNamespace::Scalar s271;
        s271.setByteString("ab", 2, false /*attachValues*/);
        arrayElements27.push_back(&s271);
        IODataProviderNamespace::Scalar s272;
        s272.setByteString("cd", 2, false /*attachValues*/);
        arrayElements27.push_back(&s272);
        array = new IODataProviderNamespace::Array(
                IODataProviderNamespace::Scalar::BYTE_STRING, &arrayElements27, false /* attachValues */);
        IODataProviderNamespace::NodeData n17(*new IODataProviderNamespace::NodeId(1, 27 /*id*/),
                array, true /* attachValues*/);
        nodeData.push_back(&n17);

        // localizedText[] -> char[][]
        arrayElements = new std::vector<const IODataProviderNamespace::Variant*>();
        s = new IODataProviderNamespace::Scalar();
        s->setLocalizedText(new std::string("en"), new std::string("ab"), true /*attachValues*/);
        arrayElements->push_back(s);
        s = new IODataProviderNamespace::Scalar();
        s->setLocalizedText(new std::string("en"), new std::string("cd"), true /*attachValues*/);
        arrayElements->push_back(s);
        array = new IODataProviderNamespace::Array(
                IODataProviderNamespace::Scalar::LOCALIZED_TEXT, arrayElements, true /* attachValues */);
        IODataProviderNamespace::NodeData n18(*new IODataProviderNamespace::NodeId(1, 28 /*id*/),
                array, true /* attachValues*/);
        nodeData.push_back(&n18);

        // write  data        
        provider.write(nodeData, false /*sendValueChangedEvents*/);

        // do not call "close" to provider and server socket
        // because the destructors do it internally
        // only wait for end of server thread
        pthread_join(serverThread, NULL /*return*/);
    }

    TEST(ProviderBinaryIoDataProvider_BinaryIODataProvider, WriteErrors) {
        /// open a server socket
        ServerSocket serverSocket(10 /*acceptTimeout*/, 10 /*sendReceiveTimeout*/,
                true /*reuseAddress*/);
        serverSocket.open(Env::PORT1); // see config for port

        // open the data provider
        Env env;
        std::string confDir = env.getApplicationDir() + "/conf/provider/binary/";
        BinaryIODataProvider provider(true /*unitTest*/);
        provider.open(confDir);
        // try to write a value of a node (the server does not accept connections)
        std::vector<const IODataProviderNamespace::NodeData*> nodeData;
        IODataProviderNamespace::Scalar* s = new IODataProviderNamespace::Scalar();
        s->setInt(3);
        IODataProviderNamespace::NodeData nd1(*new IODataProviderNamespace::NodeId(1 /*nsIndex*/, 2 /*id*/),
                s, true /*attachValues*/);
        nodeData.push_back(&nd1);
        try {
            provider.write(nodeData, false /*sendValueChangedEvents*/);
            FAIL("");
        } catch (IODataProviderNamespace::IODataProviderException& e) {
            // the request was sent but the response cannot be read
            STRCMP_CONTAINS("Cannot write data", e.getMessage().c_str());
            STRCMP_CONTAINS("Cannot write data for nodeId ", e.getCause()->getMessage().c_str());
        }
        // close the provider
        provider.close();

        // reopen the server socket (the connection is still active and the request is in tcp cache;
        // it would be read by the server after it has accepted the connection)
        serverSocket.close();
        serverSocket.open(Env::PORT1); // see config for port

        // start a server thread
        pthread_t serverThread;
        pthread_create(&serverThread, NULL, &writeErrorsThreadRun, &serverSocket);
        timespec delay;
        delay.tv_sec = 1;
        delay.tv_nsec = 0;
        nanosleep(&delay, NULL /* remaining */);
        // open the data provider
        provider.open(confDir);
        // try to write a value of a node (the server sends a response with an error)        
        try {
            provider.write(nodeData, false /*sendValueChangedEvents*/);
            FAIL("");
        } catch (IODataProviderNamespace::IODataProviderException& e) {
            STRCMP_CONTAINS("Cannot write data", e.getMessage().c_str());
            STRCMP_CONTAINS("Received a write response for",
                    e.getCause()->getMessage().c_str());
        }
        // close the provider and wait for end of server thread
        provider.close();
        pthread_join(serverThread, NULL /*return*/);

        // close the server socket
        serverSocket.close();
    }

    TEST(ProviderBinaryIoDataProvider_BinaryIODataProvider, Call) {
        /// open a server socket
        ServerSocket serverSocket(10 /*acceptTimeout*/, 10 /*sendReceiveTimeout*/,
                true /*reuseAddress*/);
        serverSocket.open(Env::PORT1); // see config for port
        // start a server thread
        pthread_t serverThread;
        pthread_create(&serverThread, NULL, &callThreadRun, &serverSocket);
        timespec delay;
        delay.tv_sec = 1;
        delay.tv_nsec = 0;
        nanosleep(&delay, NULL /* remaining */);
        // open the data provider
        Env env;
        std::string confDir = env.getApplicationDir() + "/conf/provider/binary/";
        BinaryIODataProvider provider(true /*unitTest*/);
        provider.open(confDir);
        // the binary interface does not support namespaces 
        // => get default node properties with namespace index before calling a method
        int nsIndex = 1;
        const IODataProviderNamespace::NodeProperties* dfltNodeProps =
                provider.getDefaultNodeProperties(std::string("ns"), nsIndex);
        delete dfltNodeProps;
        // call a method (APPLICATION_ERROR)
        {
            std::vector<const IODataProviderNamespace::MethodData*> inMethodData;
            IODataProviderNamespace::NodeId objectNodeId1(nsIndex, 11 /*id*/);
            IODataProviderNamespace::NodeId methodNodeId1(nsIndex, 12 /*id*/);
            std::vector<const IODataProviderNamespace::Variant*> inputArgs;
            IODataProviderNamespace::Scalar s1;
            s1.setLong(31L);
            inputArgs.push_back(&s1);
            IODataProviderNamespace::Scalar s2;
            s2.setString(new std::string("a"), true /*attachValue*/);
            inputArgs.push_back(&s2);
            IODataProviderNamespace::MethodData md1(objectNodeId1, methodNodeId1, inputArgs);
            inMethodData.push_back(&md1);
            std::vector<IODataProviderNamespace::MethodData*>* result = provider.call(inMethodData);
            VectorScopeGuard<IODataProviderNamespace::MethodData> resultSG(result);
            CHECK_EQUAL(1, result->size());
            // the request was sent but the response cannot be read
            IODataProviderNamespace::IODataProviderException* ex = (*result)[0]->getException();
            CHECK_TRUE(ex != NULL);
            STRCMP_CONTAINS("Received a call response with application error 0x22",
                    ex->getMessage().c_str());
        }
        // call a method (SUCCESS)
        {
            std::vector<const IODataProviderNamespace::MethodData*> inMethodData;
            IODataProviderNamespace::NodeId objectNodeId1(nsIndex, 11 /*id*/);
            IODataProviderNamespace::NodeId methodNodeId1(nsIndex, 12 /*id*/);
            std::vector<const IODataProviderNamespace::Variant*> inputArgs;
            IODataProviderNamespace::Scalar s1;
            s1.setInt(21);
            inputArgs.push_back(&s1);
            IODataProviderNamespace::Scalar s2;
            s2.setFloat(2.2);
            inputArgs.push_back(&s2);
            IODataProviderNamespace::MethodData md1(objectNodeId1, methodNodeId1, inputArgs);
            inMethodData.push_back(&md1);
            std::vector<IODataProviderNamespace::MethodData*>* result = provider.call(inMethodData);
            VectorScopeGuard<IODataProviderNamespace::MethodData> resultSG(result);
            // check results
            CHECK_EQUAL(1, result->size());
            IODataProviderNamespace::MethodData& outMethodData1 = *(*result)[0];
            CHECK_TRUE(outMethodData1.getObjectNodeId().equals(objectNodeId1));
            CHECK_TRUE(outMethodData1.getMethodNodeId().equals(methodNodeId1));
            CHECK_EQUAL(2, outMethodData1.getMethodArguments().size());
            IODataProviderNamespace::Scalar& rs1 =
                    *(IODataProviderNamespace::Scalar*) outMethodData1.getMethodArguments().at(0);
            CHECK_EQUAL(22, rs1.getInt());
            IODataProviderNamespace::Scalar& rs2 =
                    *(IODataProviderNamespace::Scalar*) outMethodData1.getMethodArguments().at(1);
            DOUBLES_EQUAL(3.2, rs2.getFloat(), 0.01);
        }
    }

    TEST(ProviderBinaryIoDataProvider_BinaryIODataProvider, CallErrors) {
        /// open a server socket
        ServerSocket serverSocket(10 /*acceptTimeout*/, 10 /*sendReceiveTimeout*/,
                true /*reuseAddress*/);
        serverSocket.open(Env::PORT1); // see config for port

        // open the data provider
        Env env;
        std::string confDir = env.getApplicationDir() + "/conf/provider/binary/";
        BinaryIODataProvider provider(true /*unitTest*/);
        provider.open(confDir);
        // try to call a method (the server does not accept connections)
        std::vector<const IODataProviderNamespace::NodeData*> nodeData;
        IODataProviderNamespace::Scalar* s = new IODataProviderNamespace::Scalar();
        s->setInt(3);
        IODataProviderNamespace::NodeData nd1(
                *new IODataProviderNamespace::NodeId(1 /*nsIndex*/, 2 /*id*/),
                s, true /*attachValues*/);
        nodeData.push_back(&nd1);

        std::vector<const IODataProviderNamespace::MethodData*> inMethodData;
        IODataProviderNamespace::NodeId objectNodeId1(1, 11 /*id*/);
        IODataProviderNamespace::NodeId methodNodeId1(1, 12 /*id*/);
        std::vector<const IODataProviderNamespace::Variant*> inputArgs;
        IODataProviderNamespace::Scalar s1;
        s1.setInt(21);
        inputArgs.push_back(&s1);
        IODataProviderNamespace::MethodData md1(objectNodeId1, methodNodeId1, inputArgs);
        inMethodData.push_back(&md1);
        std::vector<IODataProviderNamespace::MethodData*>* result = provider.call(inMethodData);
        VectorScopeGuard<IODataProviderNamespace::MethodData> resultSG(result);
        CHECK_EQUAL(1, result->size());
        // the request was sent but the response cannot be read
        IODataProviderNamespace::IODataProviderException* ex = (*result)[0]->getException();
        CHECK_TRUE(ex != NULL);
        // the request was sent but the response cannot be read
        STRCMP_CONTAINS("Cannot call method", ex->getMessage().c_str());
        STRCMP_CONTAINS("Cannot get message with id", ex->getCause()->getMessage().c_str());
        // close the provider
        provider.close();

        // reopen the server socket (the connection is still active and the request is in tcp cache;
        // it would be read by the server after it has accepted the connection)
        serverSocket.close();
        serverSocket.open(Env::PORT1); // see config for port

        // start a server thread
        pthread_t serverThread;
        pthread_create(&serverThread, NULL, &callErrorsThreadRun, &serverSocket);
        timespec delay;
        delay.tv_sec = 1;
        delay.tv_nsec = 0;
        nanosleep(&delay, NULL /* remaining */);
        // open the data provider
        provider.open(confDir);
        // try to call a method (the server sends a response with an error)        
        result = provider.call(inMethodData);
        VectorScopeGuard<IODataProviderNamespace::MethodData> resultSG2(result);
        CHECK_EQUAL(1, result->size());
        // the request was sent but the response cannot be read        
        ex = (*result)[0]->getException();
        CHECK_TRUE(ex != NULL);
        STRCMP_CONTAINS("Received a call response with status", ex->getMessage().c_str());
        // close the provider and wait for end of server thread
        provider.close();
        pthread_join(serverThread, NULL /*return*/);

        // close the server socket
        serverSocket.close();
    }

    TEST(ProviderBinaryIoDataProvider_BinaryIODataProvider, Subscribe) {
        /// open a server socket
        ServerSocket serverSocket(10 /*acceptTimeout*/, 10 /*sendReceiveTimeout*/,
                true /*reuseAddress*/);
        serverSocket.open(Env::PORT1); // see config for port
        // start a server thread
        pthread_t serverThread;
        pthread_create(&serverThread, NULL, &subscribeThreadRun, &serverSocket);
        timespec delay;
        delay.tv_sec = 1;
        delay.tv_nsec = 0;
        nanosleep(&delay, NULL /* remaining */);
        // open the data provider
        Env env;
        std::string confDir = env.getApplicationDir() + "/conf/provider/binary/";
        BinaryIODataProvider provider(true /*unitTest*/);
        provider.open(confDir);
        // the binary interface does not support namespaces 
        // => get default node properties with namespace index before subscribing
        int nsIndex = 1;
        const IODataProviderNamespace::NodeProperties* dfltNodeProps =
                provider.getDefaultNodeProperties(std::string("ns"), nsIndex);
        delete dfltNodeProps;
        // get time stamp
        time_t timerStampBeforeSubscribe;
        time(&timerStampBeforeSubscribe);
        // subscribe for a node
        std::vector<const IODataProviderNamespace::NodeId*> nodeIds;
        IODataProviderNamespace::NodeId n1(nsIndex, 11 /*id*/);
        nodeIds.push_back(&n1);
        IODataProviderNamespace::NodeId n2(nsIndex, 44 /*id*/); // eventTypeId        
        nodeIds.push_back(&n2);
        SubscribeSubscriberCallback sc;
        std::vector<IODataProviderNamespace::NodeData*>* results = provider.subscribe(nodeIds, sc);
        VectorScopeGuard<IODataProviderNamespace::NodeData> resultsSG(results);
        // check the results
        CHECK_EQUAL(2, results->size());
        IODataProviderNamespace::NodeData& nodeData1 = *(*results)[0];
        CHECK_TRUE(nodeData1.getNodeId().equals(n1));
        const IODataProviderNamespace::Scalar& data1 =
                *static_cast<const IODataProviderNamespace::Scalar*> (nodeData1.getData());
        CHECK_EQUAL(111, data1.getLong());
        IODataProviderNamespace::NodeData& nodeData2 = *(*results)[1];
        CHECK_TRUE(nodeData2.getNodeId().equals(n2));
        const IODataProviderNamespace::Scalar& data2 =
                *static_cast<const IODataProviderNamespace::Scalar*> (nodeData2.getData());
        CHECK_EQUAL(441, data2.getLong());
        // check the notification
        CHECK_EQUAL(2, sc.events.size());
        const std::vector<const IODataProviderNamespace::NodeData*>& notificationNodeData =
                sc.events[0]->getNodeData();
        CHECK_EQUAL(1, notificationNodeData.size());
        const IODataProviderNamespace::NodeData& nd1 = *notificationNodeData.at(0);
        const IODataProviderNamespace::Scalar& data =
                *static_cast<const IODataProviderNamespace::Scalar*> (nd1.getData());
        CHECK_EQUAL(112, data.getLong());
        // check the event
        CHECK_TRUE(timerStampBeforeSubscribe <= sc.events[1]->getDateTime() / 1000
                && sc.events[1]->getDateTime() / 1000 < timerStampBeforeSubscribe + 3);
        const std::vector<const IODataProviderNamespace::NodeData*>& eventNodeData =
                sc.events[1]->getNodeData();
        CHECK_EQUAL(1, eventNodeData.size());
        const IODataProviderNamespace::NodeData& end = *eventNodeData.at(0);
        const IODataProviderNamespace::OpcUaEventData& edata =
                *static_cast<const IODataProviderNamespace::OpcUaEventData*> (end.getData());
        CHECK_EQUAL(500, edata.getSeverity());
        STRCMP_EQUAL("huhu", edata.getMessage().c_str());
        CHECK_EQUAL(1, edata.getFieldData().size());
        const IODataProviderNamespace::NodeData& edataFieldData = *edata.getFieldData()[0];
        CHECK_TRUE(edataFieldData.getNodeId().equals(n1));
        LONGS_EQUAL(112, static_cast<const IODataProviderNamespace::Scalar*> (edataFieldData.getData())->getInt());

        provider.unsubscribe(nodeIds);
        // do not call "close" to provider and server socket
        // because the destructors do it internally
        // only wait for end of server thread
        pthread_join(serverThread, NULL /*return*/);
    }

    TEST(ProviderBinaryIoDataProvider_BinaryIODataProvider, SubscribeErrors) {
        /// open a server socket
        ServerSocket serverSocket(10 /*acceptTimeout*/, 10 /*sendReceiveTimeout*/,
                true /*reuseAddress*/);
        serverSocket.open(Env::PORT1); // see config for port

        // open the data provider
        Env env;
        std::string confDir = env.getApplicationDir() + "/conf/provider/binary/";
        BinaryIODataProvider provider(true /*unitTest*/);
        provider.open(confDir);
        // try to subscribe to a node (the server does not accept connections)
        std::vector<const IODataProviderNamespace::NodeId*> nodeIds;
        IODataProviderNamespace::NodeId n1(1 /*nsIndex*/, 2 /*id*/);
        nodeIds.push_back(&n1);
        SubscribeSubscriberCallback sc;
        std::vector<IODataProviderNamespace::NodeData*>* results = provider.subscribe(nodeIds, sc);
        VectorScopeGuard<IODataProviderNamespace::NodeData> resultsSG(results);
        CHECK_EQUAL(1, results->size());
        IODataProviderNamespace::IODataProviderException* ex = (*results)[0]->getException();
        STRCMP_CONTAINS("Cannot create a subscription for ", ex->getMessage().c_str());
        // try to unsubscribe from a node (the server does not accept connections)
        try {
            provider.unsubscribe(nodeIds);
            FAIL("");
        } catch (IODataProviderNamespace::IODataProviderException& e) {
            STRCMP_CONTAINS("Deletion of subscriptions failed", e.getMessage().c_str());
            STRCMP_CONTAINS("Cannot delete subscription for", e.getCause()->getMessage().c_str());
        }
        // close the provider and wait for end of server thread
        provider.close();

        // reopen the server socket
        serverSocket.close();
        serverSocket.open(Env::PORT1); // see config for port

        // start a server thread
        pthread_t serverThread;
        pthread_create(&serverThread, NULL, &subscribeErrorsThreadRun, &serverSocket);
        timespec delay;
        delay.tv_sec = 1;
        delay.tv_nsec = 0;
        nanosleep(&delay, NULL /* remaining */);
        // open the data provider
        provider.open(confDir);
        // try to subscribe to a node (the server sends a response with an error)        
        results = provider.subscribe(nodeIds, sc);
        VectorScopeGuard<IODataProviderNamespace::NodeData> resultsSG2(results);
        CHECK_EQUAL(1, results->size());
        ex = (*results)[0]->getException();
        STRCMP_CONTAINS("Received a subscribe response for", ex->getMessage().c_str());
        // try to unsubscribe from a node (the server sends a response with an error)        
        try {
            provider.unsubscribe(nodeIds);
            FAIL("");
        } catch (IODataProviderNamespace::IODataProviderException& e) {
            STRCMP_CONTAINS("Deletion of subscriptions failed", e.getMessage().c_str());
            STRCMP_CONTAINS("Cannot delete subscription for", e.getCause()->getMessage().c_str());
        }
        // close the provider and wait for end of server thread
        provider.close();
        pthread_join(serverThread, NULL /*return*/);

        // close the server socket
        serverSocket.close();
    }

    TEST(ProviderBinaryIoDataProvider_BinaryIODataProvider, RenewSubscription) {
        /// open a server socket
        ServerSocket serverSocket(10 /*acceptTimeout*/, 10 /*sendReceiveTimeout*/,
                true /*reuseAddress*/);
        serverSocket.open(Env::PORT1); // see config for port
        // start a server thread
        pthread_t serverThread;
        pthread_create(&serverThread, NULL, &renewSubscriptionInitThreadRun, &serverSocket);
        timespec delay;
        delay.tv_sec = 1;
        delay.tv_nsec = 0;
        nanosleep(&delay, NULL /* remaining */);
        // open the data provider
        Env env;
        std::string confDir = env.getApplicationDir() + "/conf/provider/binary/";
        BinaryIODataProvider provider(true /*unitTest*/);
        provider.open(confDir);
        // the binary interface does not support namespaces 
        // => get default node properties with namespace index before subscribing
        int nsIndex = 1;
        const IODataProviderNamespace::NodeProperties* dfltNodeProps =
                provider.getDefaultNodeProperties(std::string("ns"), nsIndex);
        delete dfltNodeProps;
        // subscribe for nodes
        std::vector<const IODataProviderNamespace::NodeId*> nodeIds;
        IODataProviderNamespace::NodeId n1(nsIndex, 11 /*id*/);
        nodeIds.push_back(&n1);
        IODataProviderNamespace::NodeId n2(nsIndex, 44 /*id*/);
        nodeIds.push_back(&n2);
        SubscribeSubscriberCallback sc;
        std::vector<IODataProviderNamespace::NodeData*>* results = provider.subscribe(nodeIds, sc);
        VectorScopeGuard<IODataProviderNamespace::NodeData> resultsSG(results);
        // check for results
        CHECK_EQUAL(2, results->size());
        // close the server socket and reopen it
        pthread_join(serverThread, NULL /*return*/);
        serverSocket.close();
        serverSocket.open(Env::PORT1);
        pthread_create(&serverThread, NULL, &renewSubscriptionThreadRun, &serverSocket);
        // the provider reconnects after 1 second and renews the subscriptions
        delay.tv_sec = 2;
        nanosleep(&delay, NULL /* remaining */);

        provider.unsubscribe(nodeIds);
        // do not call "close" to provider and server socket
        // because the destructors do it internally
        // only wait for end of server thread
        pthread_join(serverThread, NULL /*return*/);
    }

    TEST(ProviderBinaryIoDataProvider_BinaryIODataProvider, RenewSubscriptionErrors) {
        /// open a server socket
        ServerSocket serverSocket(10 /*acceptTimeout*/, 10 /*sendReceiveTimeout*/,
                true /*reuseAddress*/);
        serverSocket.open(Env::PORT1); // see config for port
        // start a server thread
        pthread_t serverThread;
        pthread_create(&serverThread, NULL, &renewSubscriptionInitThreadRun, &serverSocket);
        timespec delay;
        delay.tv_sec = 1;
        delay.tv_nsec = 0;
        nanosleep(&delay, NULL /* remaining */);
        // open the data provider
        Env env;
        std::string confDir = env.getApplicationDir() + "/conf/provider/binary/";
        BinaryIODataProvider provider(true /*unitTest*/);
        provider.open(confDir);
        // the binary interface does not support namespaces 
        // => get default node properties with namespace index before subscribing
        int nsIndex = 1;
        const IODataProviderNamespace::NodeProperties* dfltNodeProps =
                provider.getDefaultNodeProperties(std::string("ns"), nsIndex);
        delete dfltNodeProps;
        // subscribe for nodes
        std::vector<const IODataProviderNamespace::NodeId*> nodeIds;
        IODataProviderNamespace::NodeId n1(nsIndex, 11 /*id*/);
        nodeIds.push_back(&n1);
        IODataProviderNamespace::NodeId n2(nsIndex, 44 /*id*/);
        nodeIds.push_back(&n2);
        SubscribeSubscriberCallback sc;
        std::vector<IODataProviderNamespace::NodeData*>* results = provider.subscribe(nodeIds, sc);
        VectorScopeGuard<IODataProviderNamespace::NodeData> resultsSG(results);
        // check for results
        CHECK_EQUAL(2, results->size());
        // close the server socket and reopen it
        pthread_join(serverThread, NULL /*return*/);
        serverSocket.close();
        serverSocket.open(Env::PORT1);
        // receive an error status for the first unsubscribe request
        // the second unsubscribe + subscribe is successfull
        pthread_create(&serverThread, NULL, &renewSubscriptionErrorsThreadRun1, &serverSocket);
        // the provider reconnects after 1 second and renews the subscriptions
        delay.tv_sec = 2;
        nanosleep(&delay, NULL /* remaining */);

        // close the server socket and reopen it
        pthread_join(serverThread, NULL /*return*/);
        serverSocket.close();
        serverSocket.open(Env::PORT1);
        // receive an error status for the first unsubscribe request
        // the second unsubscribe + subscribe is successfull
        pthread_create(&serverThread, NULL, &renewSubscriptionErrorsThreadRun2, &serverSocket);
        // the provider reconnects after 1 second and renews the subscriptions
        delay.tv_sec = 2;
        nanosleep(&delay, NULL /* remaining */);

        provider.unsubscribe(nodeIds);
        // do not call "close" to provider and server socket
        // because the destructors do it internally
        // only wait for end of server thread
        pthread_join(serverThread, NULL /*return*/);
    }

    TEST(ProviderBinaryIoDataProvider_BinaryIODataProvider, GetNodeProperties) {
        /// open a server socket
        ServerSocket serverSocket(10 /*acceptTimeout*/, 10 /*sendReceiveTimeout*/,
                true /*reuseAddress*/);
        serverSocket.open(Env::PORT1); // see config for port

        // open the data provider
        Env env;
        std::string confDir = env.getApplicationDir() + "/conf/provider/binary/";
        BinaryIODataProvider provider(true /*unitTest*/);
        provider.open(confDir);

        // get default node properties
        const IODataProviderNamespace::NodeProperties* dfltNodeProps =
                provider.getDefaultNodeProperties(std::string("ns"), 2 /*nsIndex*/);
        CHECK_TRUE(dfltNodeProps != NULL);
        delete dfltNodeProps;

        // get node properties
        std::vector<const IODataProviderNamespace::NodeData*>* nodeProps =
                provider.getNodeProperties(std::string("ns"), 2 /*nsIndex*/);
        VectorScopeGuard<const IODataProviderNamespace::NodeData> nodePropsSG(nodeProps);
        CHECK_TRUE(nodeProps != NULL);
        // close the provider and server socket
        provider.close();
        serverSocket.close();
    }
}
