#ifndef NATIVE_NATIVE2J
#define NATIVE_NATIVE2J

#include <unistd.h>
#include <string>
#include <sstream>
#include <jni.h>
#include <jni_md.h>
#include <common/logging/ConsoleLoggerFactory.h>
#include <common/logging/LoggerFactory.h>
#include <common/Exception.h>
#include <provider/binary/messages/dto/ReadResponse.h>
#include <provider/binary/messages/dto/CallResponse.h>
#include <provider/binary/messages/dto/Event.h>
#include <provider/binary/messages/dto/Notification.h>
#include <provider/binary/messages/dto/Array.h>
#include <provider/binary/messages/dto/Struct.h>
#include <provider/binary/messages/dto/ParamList.h>
#include <provider/binary/messages/dto/StatusMessage.h>
#include <pthread.h>

#include "../../../include/common/ModelType.h"
#include "MessageHandler.h"

using namespace CommonNamespace;
using namespace std;

class Native2J : public MessageHandler{
    private:
    class ValueChanged {
        private:
            jobject key;
            jobject value;
        public:
            void setKey(jobject key){
                this->key = key;
            }
            void setValue(jobject value) {
                this->value = value;
            }
            jobject getKey() {
                return key;
            }
            jobject getValue() {
                return value;
            }
    };

    Logger* log;
    JavaVM *jvm;
    
    jobject handler;
    jclass opcua_MessageHandler;
    std::string serverId;

    std::map<std::string, std::map<std::string, ModelType> > currentModel;

    ModelType getDataTypeFromModel(std::string key, ModelType t);
    Scalar *guessScalar(JNIEnv *env, jobject data);


    public:
    Native2J(JNIEnv *env, jobject handler);
    ~Native2J(); // @suppress("Class has a virtual method and non-virtual destructor")
    void close();

    jobject createVariant(JNIEnv *env, ReadResponse& message);
    jobject createVariant(JNIEnv *env, CallResponse& message);
    jobject createVariant(JNIEnv *env, Event& message);
    ValueChanged createVariant(JNIEnv *env, Notification& message);

    jobject getVariant(JNIEnv *env, const Variant& value);
    jobject getScalar(JNIEnv *env, const Scalar& value);
    jobject getArray(JNIEnv *env, const Array& value);
    jobject getStruct(JNIEnv *env, const Struct& value);
    jobject getParamList(JNIEnv *env, const ParamList& value);
    jobject getJMap(JNIEnv *env, std::map<std::string, std::map<std::string, std::string> > values);

    void updateModel(std::map<std::string, std::map<std::string, ModelType> > newModel);

	ParamList *getParamList(JNIEnv *env, jobject data, std::vector<std::string>, ModelType t = ModelType());

	Variant *getVariant(JNIEnv *env, jobject data, std::string key = "", ModelType t = ModelType());
    Struct *getStructureVariant(JNIEnv *env, jobject data, std::string key="", ModelType t = ModelType());
    Array *getArrayVariant(JNIEnv *env, jobject data, ModelType t);
    Scalar *getScalarVariant(JNIEnv *env, jobject data, ModelType t);
    Scalar *getScalarVariant(JNIEnv *env, jchar data);
    
    ParamId *createParamId(JNIEnv *env, jint ns, jobject paramId);

    std::string getMapEntry(JNIEnv *env, jobject map, const std::string key);
    bool mapContains(JNIEnv *env, jobject map, const std::string key);


    void callMessageReceived(JNIEnv *env, jobject obj);
    void callValueChanged(JNIEnv *env, jobject id, jobject params);
    void callUsabilityChanged(JNIEnv *env, jobject obj, jboolean usable);
    void setServerId(std::string serverId);

    Exception* getException(JNIEnv *env);


    //Override
    void notificationReceived(Message& notification);
    void eventReceived(Message& event);
    void connectionStateChanged(int state);
    void modelUpdated(std::map<std::string, std::map<std::string, ModelType> > newModel);

};
#endif
