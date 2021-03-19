#include <jni.h>
#include <jni_md.h>
#include <common/logging/JLogger.h>

#include "../../utilities/linux.h" //getTimeStamp
#include <stdarg.h> // va_list
#include <stdio.h> // vasprintf
#include <stdlib.h> // free
#include <string.h> // strdup

using namespace CommonNamespace;

class JLoggerPrivate {
    friend class JLogger;
private:
    static const char LINE_DELIMITER[];

    std::string name;
};

const char JLoggerPrivate::LINE_DELIMITER[] = "\n";

int JLogger::currentLevel = 10;

JLogger::JLogger(const char* name, JNIEnv *env) {

    d = new JLoggerPrivate();
    d->name = std::string(name);
    env->GetJavaVM(&jvm);

    //Initialize Level only once
    if (JLogger::currentLevel == 10){
		jclass java_util_logging_Logger =  env->FindClass("java/util/logging/Logger");

		jclass havis_util_opcua_OPCUA =  env->FindClass("havis/util/opcua/OPCUA");
		jfieldID havis_util_opcua_OPCUA_log = env->GetStaticFieldID(havis_util_opcua_OPCUA, "log", "Ljava/util/logging/Logger;");
		jobject havis_util_opcua_OPCUA_log_ = env->GetStaticObjectField(havis_util_opcua_OPCUA, havis_util_opcua_OPCUA_log);

		jmethodID java_util_logging_Logger_isLoggable = env->GetMethodID(java_util_logging_Logger, "isLoggable", "(Ljava/util/logging/Level;)Z");

		jclass java_util_logging_Level =  env->FindClass("java/util/logging/Level");

		jfieldID java_util_logging_Level_FIELD = env->GetStaticFieldID(java_util_logging_Level, "FINEST", "Ljava/util/logging/Level;");
		jobject java_util_logging_Level_FIELD_ = env->GetStaticObjectField(java_util_logging_Level, java_util_logging_Level_FIELD);
		jboolean level = env->CallBooleanMethod(havis_util_opcua_OPCUA_log_, java_util_logging_Logger_isLoggable, java_util_logging_Level_FIELD_);
		if (level == true){
			JLogger::currentLevel = 1;
			return;
		}

		java_util_logging_Level_FIELD = env->GetStaticFieldID(java_util_logging_Level, "FINER", "Ljava/util/logging/Level;");
		java_util_logging_Level_FIELD_ = env->GetStaticObjectField(java_util_logging_Level, java_util_logging_Level_FIELD);
		level = env->CallBooleanMethod(havis_util_opcua_OPCUA_log_, java_util_logging_Logger_isLoggable, java_util_logging_Level_FIELD_);
		if (level == true){
			JLogger::currentLevel = 2;
			return;
		}

		java_util_logging_Level_FIELD = env->GetStaticFieldID(java_util_logging_Level, "FINE", "Ljava/util/logging/Level;");
		java_util_logging_Level_FIELD_ = env->GetStaticObjectField(java_util_logging_Level, java_util_logging_Level_FIELD);
		level = env->CallBooleanMethod(havis_util_opcua_OPCUA_log_, java_util_logging_Logger_isLoggable, java_util_logging_Level_FIELD_);
		if (level == true){
			JLogger::currentLevel = 3;
			return;
		}

		java_util_logging_Level_FIELD = env->GetStaticFieldID(java_util_logging_Level, "INFO", "Ljava/util/logging/Level;");
		java_util_logging_Level_FIELD_ = env->GetStaticObjectField(java_util_logging_Level, java_util_logging_Level_FIELD);
		level = env->CallBooleanMethod(havis_util_opcua_OPCUA_log_, java_util_logging_Logger_isLoggable, java_util_logging_Level_FIELD_);
		if (level == true){
			JLogger::currentLevel = 4;
			return;
		}

		java_util_logging_Level_FIELD = env->GetStaticFieldID(java_util_logging_Level, "WARNING", "Ljava/util/logging/Level;");
		java_util_logging_Level_FIELD_ = env->GetStaticObjectField(java_util_logging_Level, java_util_logging_Level_FIELD);
		level = env->CallBooleanMethod(havis_util_opcua_OPCUA_log_, java_util_logging_Logger_isLoggable, java_util_logging_Level_FIELD_);
		if (level == true){
			JLogger::currentLevel = 5;
			return;
		}

		java_util_logging_Level_FIELD = env->GetStaticFieldID(java_util_logging_Level, "SEVERE", "Ljava/util/logging/Level;");
		java_util_logging_Level_FIELD_ = env->GetStaticObjectField(java_util_logging_Level, java_util_logging_Level_FIELD);
		level = env->CallBooleanMethod(havis_util_opcua_OPCUA_log_, java_util_logging_Logger_isLoggable, java_util_logging_Level_FIELD_);
		if (level == true){
			JLogger::currentLevel = 6;
			return;
		}
   }
}

JLogger::~JLogger() {
    delete d;
}

void JLogger::error(const char* format, ...) {
    if (isErrorEnabled()) {
		char* buffer;
		va_list args;
		va_start(args, format);
		vasprintf(&buffer, format, args);
		va_end(args);
		log(buffer, "severe");
    }
}

void JLogger::warn(const char* format, ...) {
    if (isWarnEnabled()) {
		char* buffer;
		va_list args;
		va_start(args, format);
		vasprintf(&buffer, format, args);
		va_end(args);
		log(buffer, "warning");
    }
}

void JLogger::info(const char* format, ...) {
    if (isInfoEnabled()) {
        char* buffer;
        va_list args;
        va_start(args, format);
        vasprintf(&buffer, format, args);
        va_end(args);
        log(buffer, "info");
    }
}

void JLogger::debug(const char* format, ...) {
    if (isDebugEnabled()) {
		char* buffer;
		va_list args;
		va_start(args, format);
		vasprintf(&buffer, format, args);
		va_end(args);
		log(buffer, "fine");
    }
}

void JLogger::trace(const char* format, ...) {
    if (isTraceEnabled()) {
		char* buffer;
		va_list args;
		va_start(args, format);
		vasprintf(&buffer, format, args);
		va_end(args);
		log(buffer, "finer");
    }
}

void JLogger::log(char *buffer, const char *method) {
	JNIEnv *tmpEnv;
	bool detach = false;

	if (jvm->GetEnv((void **) &tmpEnv, JNI_VERSION_1_8) == JNI_EDETACHED){
		jvm->AttachCurrentThread((void **) &tmpEnv, NULL);
		detach = true;
	}

	jclass java_util_logging_Logger =  tmpEnv->FindClass("java/util/logging/Logger");
	jclass havis_util_opcua_OPCUA =  tmpEnv->FindClass("havis/util/opcua/OPCUA");

	jfieldID havis_util_opcua_OPCUA_log = tmpEnv->GetStaticFieldID(havis_util_opcua_OPCUA, "log", "Ljava/util/logging/Logger;");
	jobject havis_util_opcua_OPCUA_log_ = tmpEnv->GetStaticObjectField(havis_util_opcua_OPCUA, havis_util_opcua_OPCUA_log);

	jstring message = tmpEnv->NewStringUTF(buffer);

	jmethodID java_util_logging_Logger_METHOD = tmpEnv->GetMethodID(java_util_logging_Logger, method, "(Ljava/lang/String;)V");
	tmpEnv->CallObjectMethod(havis_util_opcua_OPCUA_log_, java_util_logging_Logger_METHOD, message);
	tmpEnv->ReleaseStringUTFChars(message, buffer);

	if (detach){
		jvm->DetachCurrentThread();
	}

}

bool JLogger::isErrorEnabled() {
	//Severe
	if (JLogger::currentLevel < 7){
		return true;
	}
	return false;
}

bool JLogger::isWarnEnabled() {
	//Warning
	if (JLogger::currentLevel < 6){
		return true;
	}
	return false;
}

bool JLogger::isInfoEnabled() {
	//Info
	if (JLogger::currentLevel < 5){
		return true;
	}
	return false;
}

bool JLogger::isDebugEnabled() {
	//Fine
	if (JLogger::currentLevel < 4){
		return true;
	}
	return false;
}

bool JLogger::isTraceEnabled() {
	//Finer and Finest
	if (JLogger::currentLevel < 3){
		return true;
	}
	return false;
}
