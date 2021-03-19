#include "havis_util_opcua_OPCUA.h"
#include <jni.h>

#include <string.h>

#include <common/Exception.h>
#include <common/logging/LoggerFactory.h>
#include <common/native2J/Native2J.h>
#include "provider/binary/messages/dto/Read.h"
#include "provider/binary/messages/dto/Call.h"
#include "provider/binary/messages/dto/Scalar.h"
#include "provider/binary/messages/dto/Write.h"
#include "provider/binary/messages/dto/Subscribe.h"
#include "provider/binary/messages/dto/Unsubscribe.h"
#include "provider/binary/messages/dto/Array.h"
#include "provider/binary/messages/dto/ParamId.h"

#include <stdlib.h>
#include <stdio.h>

#include <common/logging/JLogger.h>
#include <common/logging/JLoggerFactory.h>
#include "../../binaryServer/Client.h"

#define LIB_EXCEPTION "havis/util/opcua/OPCUAException"
#define HOST_KEY "host"
#define PORT_KEY "port"
#define USER "username"
#define PASSWD "password"
#define PUBLISH_INTERVAL "publishInterval"
#define CONNECT_TIMEOUT "connectTimeout"
#define CONNECT_TIMEOUT "connectTimeout"
#define SEND_RECIEVE_TIMEOUT "sendReceiveTimeout"
#define WATCHDOG_INTERVAL "watchdogInterval"

using namespace CommonNamespace;

static JLoggerFactory jLoggerFactory;
static Logger* logger;
static Native2J *native2j;
static Client *server;
static std::string connection;
static bool isOpened = false;


/*
 * Class:     havis_util_opcua_OPCUA
 * Method:    open
 * Signature: (Ljava/util/Map;Lhavis/util/opcua/MessageHandler;)V
 */
JNIEXPORT void JNICALL Java_havis_util_opcua_OPCUA_open(JNIEnv *env, jobject obj,
		jobject properties, jobject handler) {
	server = new Client();
	static Client::Options options;
	jLoggerFactory.setEnv(env);
	LoggerFactory loggerFactory(jLoggerFactory);

	logger = LoggerFactory::getLogger("JOPCUA");
	native2j = new Native2J(env, handler);

	std::string host = native2j->getMapEntry(env, properties,
			std::string(HOST_KEY));
	if (!host.empty()) {
		options.remoteHost = new std::string();
		*options.remoteHost = host;
		connection = host;
	} else {
		env->ThrowNew(env->FindClass(LIB_EXCEPTION),
				"Failed to open Server connection. No Host given!");
		return;
	}

	std::string port = native2j->getMapEntry(env, properties,
			std::string(PORT_KEY));
	if (!port.empty()) {
		options.remotePort = new int;
		*options.remotePort = atoi(port.c_str());
		connection += ":" + port;
	}

	std::string user = native2j->getMapEntry(env, properties,
			std::string(USER));
	if (!user.empty()) {
		options.username = new std::string();
		*options.username = user;
	}

	std::string password = native2j->getMapEntry(env, properties,
			std::string(PASSWD));
	if (!user.empty()) {
		options.password = new std::string();
		*options.password = password;
	}

	std::string publishInterval = native2j->getMapEntry(env, properties,
			std::string(PUBLISH_INTERVAL));
	if (!publishInterval.empty()) {
		options.publishInterval = new int;
		*options.publishInterval = atoi(publishInterval.c_str());
	}

	std::string connectTimeout = native2j->getMapEntry(env, properties,
			std::string(CONNECT_TIMEOUT));
	if (!connectTimeout.empty()) {
		options.connectTimeout = new int;
		*options.connectTimeout = atoi(connectTimeout.c_str());
	}

	std::string sendReceiveTimeout = native2j->getMapEntry(env, properties,
			std::string(SEND_RECIEVE_TIMEOUT));
	if (!sendReceiveTimeout.empty()) {
		options.sendReceiveTimeout = new int;
		*options.sendReceiveTimeout = atoi(sendReceiveTimeout.c_str());
	}

	std::string watchdogInterval = native2j->getMapEntry(env, properties,
				std::string(WATCHDOG_INTERVAL));
		if (!watchdogInterval.empty()) {
			options.watchdogInterval = new int;
			*options.watchdogInterval = atoi(watchdogInterval.c_str());
		}

	logger->debug("Open Connection to server %s", options.remoteHost->c_str());
	if (options.remotePort != NULL) {
		logger->debug("Open Port %d", *options.remotePort);
	}

	try {
		native2j->setServerId(connection);
		server->open(options, native2j);
		std: string serverId = connection + "@" + server->getSessionId();
		native2j->setServerId(serverId);
	} catch (Exception &e) {
		std::string st;
		e.getStackTrace(st);
		logger->error("Exception: %s", st.c_str());
		env->ThrowNew(env->FindClass(LIB_EXCEPTION),
				(std::string("Failed to read data: ") + st).c_str());
		return;
	}
	isOpened = true;
}

/*
 * Class:     havis_util_opcua_OPCUA
 * Method:    read
 * Signature: (Ljava/lang/String;[Ldata/Variant;)Ldata/Variant;
 */
JNIEXPORT jobject JNICALL Java_havis_util_opcua_OPCUA_read(JNIEnv *env, jobject obj,
		jint ns, jobject id) {
	if (isOpened) {
		try {
			ParamId *paramId = native2j->createParamId(env, ns, id);
			if (paramId != NULL) {
				Read read(9999, *paramId, true);
				ReadResponse msg = server->read(read);
				return native2j->createVariant(env, msg);
			} else {
				throw std::invalid_argument("Can not parse ParamID");
			}
		} catch (Exception &e) {
			std::string st;
			e.getStackTrace(st);
			logger->error("Exception: %s", st.c_str());
			env->ThrowNew(env->FindClass(LIB_EXCEPTION),
					(std::string("Failed to read data: ") + st).c_str());
			return env->NewGlobalRef(NULL);
		}

	} else {
		env->ThrowNew(env->FindClass(LIB_EXCEPTION),
				"The device connection is not established. Operation not possible.");
		return env->NewGlobalRef(NULL);
	}
}

/*
 * Class:     havis_util_opcua_OPCUA
 * Method:    exec
 * Signature: (ILjava/lang/Object;ILjava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_havis_util_opcua_OPCUA_exec(JNIEnv *env, jobject obj,
		jint objNs, jobject objId, jint metNs, jobject metId, jobject params) {
	if (isOpened) {
		try {
			ParamId *objectId = native2j->createParamId(env, objNs, objId);
			ParamId *methodId = native2j->createParamId(env, metNs, metId);
			if (objectId != NULL && methodId != NULL) {
				ModelType t;
				t.type = ModelType::REF;
				t.ref = methodId->toString();
				ParamList* list = native2j->getParamList(env, params, server->checkModel(*methodId, 0), t);
				Call call(9999, *methodId, *objectId, *list, true);
				CallResponse msg = server->call(call);
				return native2j->createVariant(env, msg);
			}
		} catch (Exception &e) {
			std::string st;
			e.getStackTrace(st);
			logger->error("Exception: %s", st.c_str());
			env->ThrowNew(env->FindClass(LIB_EXCEPTION),
					(std::string("Failed to exec: ") + st).c_str());
			return env->NewGlobalRef(NULL);
		}
	} else {
		env->ThrowNew(env->FindClass(LIB_EXCEPTION),
				"The device connection is not established. Operation not possible.");
		return env->NewGlobalRef(NULL);
	}
}

/*
 * Class:     havis_util_opcua_OPCUA
 * Method:    write
 * Signature: (Ljava/lang/String;[Ldata/Variant;)V
 */
JNIEXPORT void JNICALL Java_havis_util_opcua_OPCUA_write(JNIEnv *env, jobject obj, jint ns,
		jobject id, jobject params) {
	if (isOpened) {
		try {
			ParamId *paramId = native2j->createParamId(env, ns, id);
			if (paramId != NULL) {
				ModelType t;
				t.type = ModelType::REF;
				t.ref = paramId->toString();
				Variant * var = native2j->getVariant(env, params, server->checkModel(*paramId, 1).at(0), t);
				Write write(9999, *paramId, *var, true);
				server->write(write);
			}
		} catch (Exception &e) {
			std::string st;
			e.getStackTrace(st);
			logger->error("Exception: %s", st.c_str());
			env->ThrowNew(env->FindClass(LIB_EXCEPTION),
					(std::string("Failed to write data: ") + st).c_str());
			return;
		}
	} else {
		env->ThrowNew(env->FindClass(LIB_EXCEPTION),
				"The device connection is not established. Operation not possible.");
		return;
	}
}

/*
 * Class:     havis_util_opcua_OPCUA
 * Method:    subscribe
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_havis_util_opcua_OPCUA_subscribe(JNIEnv *env, jobject obj,
		jint ns, jobject id) {
	if (isOpened) {
		try {
			ParamId *paramId = native2j->createParamId(env, ns, id);
			if (paramId != NULL) {
				Subscribe s(9999, *paramId, true);
				server->subscribe(s);
			}
		} catch (Exception &e) {
			std::string st;
			e.getStackTrace(st);
			logger->error("Exception: %s", st.c_str());
			env->ThrowNew(env->FindClass(LIB_EXCEPTION),
					(std::string("Failed to subscribe: ") + st).c_str());
			return;
		}
	} else {
		env->ThrowNew(env->FindClass(LIB_EXCEPTION),
				"The device connection is not established. Operation not possible.");
		return;
	}

}

/*
 * Class:     havis_util_opcua_OPCUA
 * Method:    unsubscribe
 * Signature: (ILjava/lang/Object;)V
 */
JNIEXPORT void JNICALL Java_havis_util_opcua_OPCUA_unsubscribe(JNIEnv *env, jobject obj,
		jint ns, jobject id) {
	if (isOpened) {
		try {
			ParamId *paramId = native2j->createParamId(env, ns, id);
			if (paramId != NULL) {
				Unsubscribe s(9999, *paramId, true);
				server->unsubscribe(s);
			}
		} catch (Exception &e) {
			std::string st;
			e.getStackTrace(st);
			logger->error("Exception: %s", st.c_str());
			env->ThrowNew(env->FindClass(LIB_EXCEPTION),
					(std::string("Failed to unsubscribe: ") + st).c_str());
			return;
		}
	} else {
		env->ThrowNew(env->FindClass(LIB_EXCEPTION),
				"The device connection is not established. Operation not possible.");
		return;
	}
}

/*
 * Class:     havis_util_opcua_OPCUA
 * Method:    close
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_havis_util_opcua_OPCUA_close(JNIEnv *env, jobject obj) {
	if (isOpened) {
		logger->debug("Close Client");
		server->close();
		isOpened = false;
	}
}

/*
 * Class:     havis_util_opcua_OPCUA
 * Method:    browse
 * Signature: ()Ljava/util/Map;
 */
JNIEXPORT jobject JNICALL Java_havis_util_opcua_OPCUA_browse(JNIEnv *env, jobject obj,
		jint ns, jobject nodeId, jstring prefix) {
	jobject value = env->NewGlobalRef(NULL);
	if (isOpened) {
		ParamId *paramId = native2j->createParamId(env, ns, nodeId);
		const char *cprefix = env->GetStringUTFChars(prefix, NULL);
		std::string prefix(cprefix);

		env->ReleaseStringUTFChars((jstring) nodeId, cprefix);

		if (paramId != NULL) {
			switch (paramId->getParamIdType()) {
			case ParamId::NUMERIC:
				value = native2j->getJMap(env,
						server->browse(ns, paramId->getNumeric(), prefix));
				delete paramId;
				return value;
				break;
			case ParamId::STRING:
				std::string node(paramId->getString().c_str());
				value = native2j->getJMap(env, server->browse(ns, node, prefix));
				delete paramId;
				return value;
				break;
			}
		}
		return value;
	} else {
		env->ThrowNew(env->FindClass(LIB_EXCEPTION),
				"The device connection is not established. Operation not possible.");
		return value;
	}

}

