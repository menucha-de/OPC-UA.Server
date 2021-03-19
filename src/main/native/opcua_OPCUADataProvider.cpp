#include "havis_util_opcua_OPCUADataProvider.h"

#include "Server.h"
#include <common/logging/LoggerFactory.h>
#include <common/logging/JLogger.h>
#include <common/logging/JLoggerFactory.h>

#define LIB_EXCEPTION "havis/util/opcua/OPCUAException"

using namespace CommonNamespace;

static Logger* haLog;
static JLoggerFactory jLoggerFactory;
static Server* server;
/*
 * Class:     havis_util_opcua_OPCUADataProvider
 * Method:    open
 * Signature: (Ljava/util/Map;Lhavis/util/opcua/DataProvider;)V
 */
JNIEXPORT void JNICALL Java_havis_util_opcua_OPCUADataProvider_open
  (JNIEnv *env, jobject obj, jobject properties, jobject dataProvider){
	server = new Server(0);
	jLoggerFactory.setEnv(env);
	LoggerFactory loggerFactory(jLoggerFactory);
	haLog = LoggerFactory::getLogger("JOPCUA-DATAPROVIDER");
	try {
		server->open(env, properties, dataProvider);
	} catch (Exception &e) {
		std::string st;
		e.getStackTrace(st);
		haLog->error("Exception: %s", st.c_str());
		env->ThrowNew(env->FindClass(LIB_EXCEPTION),
				(std::string("Failed open server: ") + st).c_str());
		return;
	}
}

/*
 * Class:     havis_util_opcua_OPCUADataProvider
 * Method:    send
 * Signature: (Ljava/lang/Object;)V
 */
JNIEXPORT void JNICALL Java_havis_util_opcua_OPCUADataProvider_notification
  (JNIEnv *env, jobject obj, jint ns, jobject id, jobject msg){
	server->notification(env, ns, id, msg);

}

/*
 * Class:     havis_util_opcua_OPCUADataProvider
 * Method:    send
 * Signature: (Ljava/lang/Object;)V
 */
JNIEXPORT void JNICALL Java_havis_util_opcua_OPCUADataProvider_event
  (JNIEnv *env, jobject obj, jint eNs, jobject evt, jint pNs, jobject param, jlong timestamp ,jint severity, jstring msg, jobject value){
	server->event(env, eNs, evt, pNs, param, timestamp, severity, msg, value);
}

