#include "Native2J.h"

#include <common/logging/JLogger.h>
#include <common/logging/JLoggerFactory.h>

#include <uadatavalue.h>

#define LIB_EXCEPTION "havis/util/opcua/OPCUAException"

Native2J::Native2J(JNIEnv *env, jobject handler) {
	this->log = LoggerFactory::getLogger("Native2J");
	env->GetJavaVM(&jvm);
	this->handler = env->NewGlobalRef(handler);
	opcua_MessageHandler = (jclass) env->NewGlobalRef(
			env->FindClass("havis/util/opcua/MessageHandler"));
	serverId = "";
}

Native2J::~Native2J() {
	serverId = "";
}

void Native2J::updateModel(std::map<std::string, std::map<std::string, ModelType> > newModel) {
	currentModel = newModel;
}

void Native2J::modelUpdated(std::map<std::string, std::map<std::string, ModelType> > newModel){
	currentModel = newModel;
}

void Native2J::callMessageReceived(JNIEnv *env, jobject msg) {
	jmethodID opcua_MessageHandler_messageReceived = env->GetMethodID(
			opcua_MessageHandler, "messageReceived", "(Ljava/lang/Object;)V");
	env->CallVoidMethod(handler, opcua_MessageHandler_messageReceived, msg);
	env->DeleteLocalRef(msg);
}

void Native2J::callValueChanged(JNIEnv *env, jobject id, jobject params) {
	jmethodID opcua_MessageHandler_valueChanged = env->GetMethodID(
			opcua_MessageHandler, "valueChanged",
			"(Ljava/lang/Object;Ljava/lang/Object;)V");
	env->CallVoidMethod(handler, opcua_MessageHandler_valueChanged, id, params);
	env->DeleteLocalRef(id);
	env->DeleteLocalRef(params);
}

void Native2J::callUsabilityChanged(JNIEnv *env, jobject obj, jboolean usable) {
	jmethodID opcua_MessageHandler_usabilityChanged = env->GetMethodID(
			opcua_MessageHandler, "usabilityChanged", "(Ljava/lang/Object;Z)V");
	env->CallVoidMethod(handler, opcua_MessageHandler_usabilityChanged, obj,
			usable);
	env->DeleteLocalRef(obj);
}

std::string Native2J::getMapEntry(JNIEnv *env, jobject map,
		const std::string key) {
	jclass java_util_HashMap = env->FindClass("java/util/HashMap");
	jmethodID java_util_HashMap_get = env->GetMethodID(java_util_HashMap, "get",
			"(Ljava/lang/Object;)Ljava/lang/Object;");
	jstring jKey = env->NewStringUTF(key.c_str());
	jstring jValue = (jstring) env->CallObjectMethod(map, java_util_HashMap_get,
			jKey);
	const char *cstr = NULL;
	std::string result;
	if (jValue != NULL) {
		cstr = env->GetStringUTFChars(jValue, NULL);
		result = std::string(cstr);
		env->ReleaseStringUTFChars(jValue, cstr);
	}

	env->DeleteLocalRef(jKey);
	env->DeleteLocalRef(jValue);
	env->DeleteLocalRef(java_util_HashMap);
	return result;
}

bool Native2J::mapContains(JNIEnv *env, jobject map, const std::string key) {
	jclass java_util_HashMap = env->FindClass("java/util/HashMap");
	jmethodID java_util_HashMap_containsKey = env->GetMethodID(java_util_HashMap, "containsKey",
				"(Ljava/lang/Object;)Z");
	jstring jKey = env->NewStringUTF(key.c_str());
	jboolean result = env->CallBooleanMethod(map, java_util_HashMap_containsKey, jKey);
	return result;
}

jobject Native2J::createVariant(JNIEnv *env, ReadResponse& message) {
	ReadResponse& msg = (ReadResponse&) message;
	if (msg.getStatus() != Status::SUCCESS) {
		std::stringstream ss;
		ss << "Read Operation failed, status= " << msg.getStatus();
		env->ThrowNew(env->FindClass(LIB_EXCEPTION), ss.str().c_str());
		return env->NewGlobalRef(NULL);
	}
	return getVariant(env, *msg.getParamValue());
}

jobject Native2J::createVariant(JNIEnv *env, CallResponse& message) {
	CallResponse& msg = (CallResponse&) message;
	if (msg.getStatus() != Status::SUCCESS) {
		std::stringstream ss;
		ss << "Call failed, status= " << msg.getStatus();
		env->ThrowNew(env->FindClass(LIB_EXCEPTION), ss.str().c_str());
		return env->NewGlobalRef(NULL);
	}
	return getParamList(env, *msg.getParamList());
}

jobject Native2J::createVariant(JNIEnv *env, Event& message) {
	jclass java_lang_Integer = env->FindClass("java/lang/Integer");
	jmethodID java_lang_Integer_ = env->GetMethodID(java_lang_Integer, "<init>",
			"(I)V");

	jclass java_lang_Long = env->FindClass("java/lang/Long");
	jmethodID java_lang_Long_ = env->GetMethodID(java_lang_Long, "<init>",
			"(J)V");

	jclass java_util_HashMap = env->FindClass("java/util/HashMap");
	jmethodID java_util_HashMap_ = env->GetMethodID(java_util_HashMap, "<init>",
			"()V");
	jmethodID java_util_HashMap_put = env->GetMethodID(java_util_HashMap, "put",
			"(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");

	jobject map = env->NewObject(java_util_HashMap, java_util_HashMap_);
	jobject innerMap = env->NewObject(java_util_HashMap, java_util_HashMap_);

	ParamMap paramMap = message.getParamMap();
	const std::map<const ParamId*, const Variant*>& elements =
			paramMap.getElements();

	env->CallObjectMethod(innerMap, java_util_HashMap_put,
			env->NewStringUTF("message"),
			env->NewStringUTF(message.getMessage().c_str()));

	env->CallObjectMethod(innerMap, java_util_HashMap_put,
			env->NewStringUTF("timestamp"),
			env->NewObject(java_lang_Long, java_lang_Long_,
					(jlong) message.getTimeStamp()));

	env->CallObjectMethod(innerMap, java_util_HashMap_put,
			env->NewStringUTF("severity"),
			env->NewObject(java_lang_Integer, java_lang_Integer_,
					(jint) message.getSeverity()));

	for (std::map<const ParamId*, const Variant*>::const_iterator i =
			elements.begin(); i != elements.end(); i++) {
		const ParamId& paramId = *(*i).first;
		const Variant& paramValue = *(*i).second;
		jobject id = env->NewStringUTF(paramId.toString().c_str());
		env->CallObjectMethod(innerMap, java_util_HashMap_put, id,
				getVariant(env, paramValue));

	}

	const ParamId& paramId = message.getEventId();
	jobject id = env->NewStringUTF(paramId.toString().c_str());
	env->CallObjectMethod(map, java_util_HashMap_put, id, innerMap);
	return map;
}

Native2J::ValueChanged Native2J::createVariant(JNIEnv *env,
		Notification& message) {
	ValueChanged value;
	ParamMap paramMap = message.getParamMap();
	const std::map<const ParamId*, const Variant*>& elements =
			paramMap.getElements();
	for (std::map<const ParamId*, const Variant*>::const_iterator i =
			elements.begin(); i != elements.end(); i++) {
		const ParamId& paramId = *(*i).first;
		const Variant& paramValue = *(*i).second;
		jobject id = env->NewStringUTF(paramId.toString().c_str());
		value.setKey(id);
		value.setValue(getVariant(env, paramValue));
		break;
	}
	return value;
}

ParamList *Native2J::getParamList(JNIEnv *env, jobject data, std::vector<std::string> names, ModelType t) {
	jobjectArray arr = (jobjectArray) data;
	std::vector<const Variant*>* elements = new std::vector<const Variant*>();
	if (data != NULL) {
		int len = env->GetArrayLength(arr);
		for (int i = 0; i < len; i++) {
			jobject value = env->GetObjectArrayElement(arr, i);
			if (names.size() > i){
				elements->push_back(getVariant(env, value, names.at(i), t));
			} else {
				elements->push_back(getVariant(env, value, "", t));
			}
		}
	}
	return new ParamList(*elements, true);
}

//####################### jobject -> Variant

Variant* Native2J::getVariant(JNIEnv *env, jobject data, std::string key, ModelType t) {
	jclass java_lang_Class = env->FindClass("java/lang/Class");
	jmethodID java_lang_Class_isArray = env->GetMethodID(java_lang_Class,
			"isArray", "()Z");
	jclass java_lang_Object = env->GetObjectClass(data);
	jboolean is_array = env->CallBooleanMethod(java_lang_Object,
			java_lang_Class_isArray);

	jclass java_lang_String = env->FindClass("java/lang/String");
	jclass java_util_HashMap = env->FindClass("java/util/HashMap");
	jclass java_util_ArrayList = env->FindClass("java/util/ArrayList");

	if (t.type == ModelType::REF){
		t = getDataTypeFromModel(key, t);
	}

	if (env->IsInstanceOf(data, java_util_HashMap)) {
		return getStructureVariant(env, data, key, t);
	} else if (env->IsInstanceOf(data, java_lang_String) || env->IsInstanceOf(data, java_util_ArrayList) || is_array) {
		return getArrayVariant(env, data, t);
	} else {
		return getScalarVariant(env, data, t);
	}

}

Struct* Native2J::getStructureVariant(JNIEnv *env, jobject data, std::string key, ModelType t) {
	ParamId *structId = NULL;
	if (t.type == ModelType::REF){
		structId = new ParamId(t.ref);
	} else {
		structId = new ParamId(0, t.t);
	}

	jclass java_util_HashMap = env->FindClass("java/util/HashMap");
	jmethodID java_util_HashMap_keySet = env->GetMethodID(java_util_HashMap,
			"keySet", "()Ljava/util/Set;");
	jmethodID java_util_HashMap_get = env->GetMethodID(java_util_HashMap, "get",
			"(Ljava/lang/Object;)Ljava/lang/Object;");
	jobject keySet = env->CallObjectMethod(data, java_util_HashMap_keySet);

	jclass java_util_Set = env->FindClass("java/util/Set");
	jmethodID java_util_Set_toArray = env->GetMethodID(java_util_Set, "toArray",
			"()[Ljava/lang/Object;");

	jobjectArray arr = (jobjectArray) env->CallObjectMethod(keySet,
			java_util_Set_toArray);
	int len = env->GetArrayLength(arr);

	std::map<std::string, const Variant*>* fields = new std::map<std::string,
			const Variant*>();

	for (int i = 0; i < len; i++) {
		jstring key = (jstring) env->GetObjectArrayElement(arr, i);
		const char *ckey = env->GetStringUTFChars(key, 0);
		jobject value = (jstring) env->CallObjectMethod(data,
				java_util_HashMap_get, key);
		Variant *variant;
		if (t.type == ModelType::REF){
			variant = getVariant(env, value, ckey, t);
		}
		else {
			variant = getVariant(env, value, ckey);
		}
		(*fields)[std::string(ckey)] = variant;
		env->ReleaseStringUTFChars(key, ckey);
	}

	env->DeleteLocalRef(keySet);
	env->DeleteLocalRef(arr);

	return new Struct(*structId, *fields, true);
}

Array *Native2J::getArrayVariant(JNIEnv *env, jobject data, ModelType t) {
	int arrayType = Array::STRUCT;
	std::vector<const Variant*>* elements = new std::vector<const Variant*>();
	jclass java_lang_String = env->FindClass("java/lang/String");
	if (env->IsInstanceOf(data, java_lang_String)) {
		arrayType = Scalar::CHAR;
		jmethodID java_lang_String_ToCharArray = env->GetMethodID(
				java_lang_String, "toCharArray", "()[C");
		jcharArray arr = (jcharArray) env->CallObjectMethod(data,
				java_lang_String_ToCharArray);
		int len = env->GetArrayLength(arr);
		jchar* value = env->GetCharArrayElements(arr, 0);
		for (int i = 0; i < len; i++) {
			elements->push_back(getScalarVariant(env, value[i]));
		}
		env->ReleaseCharArrayElements(arr, value, 0);
	} else {
		jobjectArray arr;
		jclass java_util_ArrayList = env->FindClass("java/util/ArrayList");
		if (env->IsInstanceOf(data, java_util_ArrayList)){
			jmethodID java_util_ArrayList_toArray = env->GetMethodID(java_util_ArrayList,
					"toArray", "()[Ljava/lang/Object;");
			data = env->CallObjectMethod(data, java_util_ArrayList_toArray);
		}
		arr = (jobjectArray) data;
		int len = env->GetArrayLength(arr);
		if (len==0){
			if (t.type == ModelType::REF || t.t == -99) {
				arrayType = Array::STRUCT;
			} else {
				arrayType = t.t;
			}
		}
		for (int i = 0; i < len; i++) {
			jobject value = env->GetObjectArrayElement(arr, i);
			Variant* inner;
			if (t.type == ModelType::REF){

			}
			inner = getVariant(env, value, "", t);
			arrayType = inner->getVariantType();
			if (arrayType == Variant::SCALAR){
				Scalar *s = (Scalar*) inner;
				arrayType = s->getScalarType();
			}
			elements->push_back(inner);
		}
		env->DeleteLocalRef(arr);
	}
	return new Array(arrayType, *elements, true);
}


Scalar *Native2J::guessScalar(JNIEnv *env, jobject data) {
	Scalar *scalar = new Scalar();

	jclass java_lang_Boolean = env->FindClass("java/lang/Boolean");
	jclass java_lang_Character = env->FindClass("java/lang/Character");
	jclass java_lang_Byte = env->FindClass("java/lang/Byte");
	jclass java_lang_Short = env->FindClass("java/lang/Short");
	jclass java_lang_Integer = env->FindClass("java/lang/Integer");
	jclass java_lang_Long = env->FindClass("java/lang/Long");
	jclass java_lang_Float = env->FindClass("java/lang/Float");
	jclass java_lang_Double = env->FindClass("java/lang/Double");

	jmethodID method;

	if (env->IsInstanceOf(data, java_lang_Boolean)) {
		method = env->GetMethodID(java_lang_Boolean, "booleanValue", "()Z");
		jboolean b = env->CallBooleanMethod(data, method);
		scalar->setBoolean(b);
	} else if (env->IsInstanceOf(data, java_lang_Character)) {
		method = env->GetMethodID(java_lang_Character, "charValue", "()C");
		jchar c = env->CallCharMethod(data, method);
		scalar->setChar(c);
	} else if (env->IsInstanceOf(data, java_lang_Byte)) {
		method = env->GetMethodID(java_lang_Byte, "byteValue", "()B");
		jbyte by = env->CallCharMethod(data, method);
		scalar->setByte(by);
	} else if (env->IsInstanceOf(data, java_lang_Short)) {
		method = env->GetMethodID(java_lang_Short, "shortValue", "()S");
		jshort s = env->CallCharMethod(data, method);
		scalar->setShort(s);
	} else if (env->IsInstanceOf(data, java_lang_Integer)) {
		method = env->GetMethodID(java_lang_Integer, "intValue", "()I");
		jint i = env->CallIntMethod(data, method);
		scalar->setInt(i);
	} else if (env->IsInstanceOf(data, java_lang_Long)) {
		method = env->GetMethodID(java_lang_Long, "longValue", "()J");
		jlong l = env->CallIntMethod(data, method);
		scalar->setLong(l);
	} else if (env->IsInstanceOf(data, java_lang_Float)) {
		method = env->GetMethodID(java_lang_Float, "floatValue", "()F");
		jfloat f = env->CallIntMethod(data, method);
		scalar->setFloat(f);
	} else if (env->IsInstanceOf(data, java_lang_Double)) {
		method = env->GetMethodID(java_lang_Double, "doubleValue", "()D");
		jdouble d = env->CallDoubleMethod(data, method);
		scalar->setDouble(d);
	}

	return scalar;
}

Scalar *Native2J::getScalarVariant(JNIEnv *env, jobject data, ModelType t) {
	Scalar *scalar = new Scalar();

	jclass java_lang_Boolean = env->FindClass("java/lang/Boolean");
	jclass java_lang_Character = env->FindClass("java/lang/Character");
	jclass java_lang_Byte = env->FindClass("java/lang/Byte");
	jclass java_lang_Short = env->FindClass("java/lang/Short");
	jclass java_lang_Integer = env->FindClass("java/lang/Integer");
	jclass java_lang_Long = env->FindClass("java/lang/Long");
	jclass java_lang_Float = env->FindClass("java/lang/Float");
	jclass java_lang_Double = env->FindClass("java/lang/Double");

	jmethodID method;
	if (t.type != ModelType::REF && t.t ==-99){
		delete scalar;
		return guessScalar(env, data);
	}
	if (t.type == ModelType::REF){
		ModelType t2 = getDataTypeFromModel(t.ref, t);
		//No SubReference?
		if (t.ref == t2.ref){
			delete scalar;
			return guessScalar(env, data);
		}
		delete scalar;
		return getScalarVariant(env, data, t2);
	} else {
		switch (t.t) {
			case OpcUaId_Int32:
			case OpcUaId_Enumeration:{
				method = env->GetMethodID(java_lang_Integer, "intValue", "()I");
				jint i = env->CallIntMethod(data, method);
				scalar->setInt(i);
				break;
			};
			case OpcUaId_Double:{
				method = env->GetMethodID(java_lang_Double, "doubleValue", "()D");
				jdouble d = env->CallDoubleMethod(data, method);
				scalar->setDouble(d);
				break;
			};
			case OpcUaId_Float:{
				method = env->GetMethodID(java_lang_Double, "doubleValue", "()D");
				jdouble d = env->CallDoubleMethod(data, method);
				scalar->setFloat(d);
				break;
			};
			case OpcUaId_ByteString: {
				method = env->GetMethodID(java_lang_Integer, "intValue", "()I");
				jint i = env->CallIntMethod(data, method);
				scalar->setByte(i);
				break;
			};
			default: {
				delete scalar;
				return guessScalar(env, data);
			}
		}
	}

	return scalar;
}

Scalar *Native2J::getScalarVariant(JNIEnv *env, jchar data) {
	Scalar *scalar = new Scalar;
	scalar->setChar(data);
	return scalar;
}

ModelType Native2J::getDataTypeFromModel(std::string key, ModelType t){
	if (currentModel.size()>0 && t.type==ModelType::REF){
		if (currentModel.count(t.ref) > 0) {
			if (currentModel[t.ref].size() > 0) {
				if (key.size()>0 && currentModel[t.ref].count(key) > 0) {
					return currentModel[t.ref][key];
				} else if (currentModel[t.ref].count(t.ref) > 0) {
					return currentModel[t.ref][t.ref];
				}
			}
		}
	}
	//No data type found in model
	return t;
}



//########################## Variant -> jobject

jobject Native2J::getVariant(JNIEnv *env, const Variant& paramValue) {
	jobject variant = env->NewGlobalRef(NULL);
	switch (paramValue.getVariantType()) {
	case Variant::SCALAR: {
		variant = getScalar(env, (Scalar&) paramValue);
		break;
	}
	case Variant::ARRAY: {
		variant = getArray(env, (Array&) paramValue);
		break;
	}
	case Variant::STRUCT: {
		variant = getStruct(env, (Struct&) paramValue);
		break;
	}
	default:
		std::stringstream ss;
		ss << "Invalid variant type " << paramValue.getVariantType();
		env->ThrowNew(env->FindClass(LIB_EXCEPTION), ss.str().c_str());
		return env->NewGlobalRef(NULL);
	}
	return variant;
}

jobject Native2J::getParamList(JNIEnv *env, const ParamList& paramList) {
	jobject variant = env->NewGlobalRef(NULL);
	std::vector<const Variant*> elements = paramList.getElements();
	jclass java_lang_Object = env->FindClass("java/lang/Object");
	jobjectArray params = env->NewObjectArray(elements.size(), java_lang_Object,
	NULL);
	int element = 0;
	for (std::vector<const Variant*>::const_iterator i = elements.begin();
			i != elements.end(); i++, element++) {
		const Variant& paramValue = *(*i);
		env->SetObjectArrayElement(params, element,
				getVariant(env, paramValue));
	}
	variant = params;
	return variant;
}

jobject Native2J::getJMap(JNIEnv *env,
		std::map<std::string, std::map<std::string, std::string> > values) {
	jclass java_util_hashMap = env->FindClass("java/util/HashMap");
	jmethodID java_util_hashMap_ = env->GetMethodID(java_util_hashMap, "<init>",
			"()V");
	jmethodID java_util_Hashmap_put = env->GetMethodID(java_util_hashMap, "put",
			"(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");

	jobject result = env->NewObject(java_util_hashMap, java_util_hashMap_);
	for (std::map<std::string, map<std::string, std::string> >::iterator it =
			values.begin(); it != values.end(); ++it) {
		jobject map = env->NewObject(java_util_hashMap, java_util_hashMap_);
		std::map<std::string, std::string> values = it->second;
		for (std::map<std::string, std::string>::iterator val = values.begin();
				val != values.end(); ++val) {
			env->CallObjectMethod(map, java_util_Hashmap_put,
					env->NewStringUTF(val->first.c_str()),
					env->NewStringUTF(val->second.c_str()));
		}
		jstring key = env->NewStringUTF(it->first.c_str());
		env->CallObjectMethod(result, java_util_Hashmap_put, key, map);
		env->DeleteLocalRef(map);
	}

	return result;
}

jobject Native2J::getStruct(JNIEnv *env, const Struct& value) {
	jclass java_lang_Integer = env->FindClass("java/lang/Integer");
	jmethodID java_lang_Integer_ = env->GetMethodID(java_lang_Integer, "<init>",
			"(I)V");
	jclass java_util_HashMap = env->FindClass("java/util/HashMap");
	jmethodID java_util_HashMap_ = env->GetMethodID(java_util_HashMap, "<init>",
			"()V");
	jmethodID java_util_HashMap_put = env->GetMethodID(java_util_HashMap, "put",
			"(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
	const std::map<std::string, const Variant*>& fields = value.getFields();

	jobject map = env->NewObject(java_util_HashMap, java_util_HashMap_);
	for (std::map<std::string, const Variant*>::const_iterator i =
			fields.begin(); i != fields.end(); i++) {
		std::string key = (*i).first;
		const Variant& paramValue = *(*i).second;
		jstring jkey = env->NewStringUTF(key.c_str());
		env->CallObjectMethod(map, java_util_HashMap_put, jkey,
				getVariant(env, paramValue));
		//env->ReleaseStringUTFChars(jkey, key.c_str());
	}
	return map;
}

jobject Native2J::getArray(JNIEnv *env, const Array& value) {
	jobject array = env->NewGlobalRef(NULL);
	const std::vector<const Variant*>& elements = value.getElements();
	if (Scalar::CHAR == value.getArrayType()) {
		char str[elements.size() + 1];
		unsigned int i;
		for (i = 0; i < elements.size(); i++) {
			const Scalar* s = static_cast<const Scalar*>(elements[i]);
			str[i] = s->getChar();
		}
		str[i] = '\0';
		array = env->NewStringUTF(str);
	} else {
		jclass java_util_ArrayList = env->FindClass("java/util/ArrayList");
		jmethodID java_util_ArrayList_ = env->GetMethodID(java_util_ArrayList,
				"<init>", "()V");
		jmethodID java_util_ArrayList_add = env->GetMethodID(
				java_util_ArrayList, "add", "(Ljava/lang/Object;)Z");
		array = env->NewObject(java_util_ArrayList, java_util_ArrayList_);
		for (std::vector<const Variant*>::const_iterator it = elements.begin();
				it != elements.end(); it++) {
			env->CallBooleanMethod(array, java_util_ArrayList_add,
					getVariant(env, **it));
		}
	}
	return array;
}

jobject Native2J::getScalar(JNIEnv *env, const Scalar& value) {
	jobject scalar = env->NewGlobalRef(NULL);
	switch (value.getScalarType()) {
	case Scalar::BOOLEAN: {
		jclass java_lang_Boolean = env->FindClass("java/lang/Boolean");
		jmethodID java_lang_Boolean_ = env->GetMethodID(java_lang_Boolean,
				"<init>", "(Z)V");
		scalar = env->NewObject(java_lang_Boolean, java_lang_Boolean_,
				value.getBoolean());
		break;
	}
	case Scalar::CHAR: {
		jclass java_lang_Character = env->FindClass("java/lang/Character");
		jmethodID java_lang_Character_ = env->GetMethodID(java_lang_Character,
				"<init>", "(C)V");
		scalar = env->NewObject(java_lang_Character, java_lang_Character_,
				value.getChar());
		break;
	}
	case Scalar::BYTE: {
		jclass java_lang_Byte = env->FindClass("java/lang/Byte");
		jmethodID java_lang_Byte_ = env->GetMethodID(java_lang_Byte, "<init>",
				"(B)V");
		scalar = env->NewObject(java_lang_Byte, java_lang_Byte_,
				value.getByte());
		break;
	}
	case Scalar::SHORT: {
		jclass java_lang_Short = env->FindClass("java/lang/Short");
		jmethodID java_lang_Short_ = env->GetMethodID(java_lang_Short, "<init>",
				"(S)V");
		scalar = env->NewObject(java_lang_Short, java_lang_Short_,
				value.getShort());
		break;
	}
	case Scalar::INT: {
		jclass java_lang_Integer = env->FindClass("java/lang/Integer");
		jmethodID java_lang_Integer_ = env->GetMethodID(java_lang_Integer,
				"<init>", "(I)V");
		scalar = env->NewObject(java_lang_Integer, java_lang_Integer_,
				value.getInt());
		break;
	}
	case Scalar::LONG: {
		jclass java_lang_Long = env->FindClass("java/lang/Long");
		jmethodID java_lang_Long_ = env->GetMethodID(java_lang_Long, "<init>",
				"(J)V");
		scalar = env->NewObject(java_lang_Long, java_lang_Long_,
				value.getLong());
		break;
	}
	case Scalar::FLOAT: {
		jclass java_lang_Float = env->FindClass("java/lang/Float");
		jmethodID java_lang_Float_ = env->GetMethodID(java_lang_Float, "<init>",
				"(F)V");
		scalar = env->NewObject(java_lang_Float, java_lang_Float_,
				value.getFloat());
		break;
	}
	case Scalar::DOUBLE: {
		jclass java_lang_Double = env->FindClass("java/lang/Double");
		jmethodID java_lang_Double_ = env->GetMethodID(java_lang_Double,
				"<init>", "(D)V");
		scalar = env->NewObject(java_lang_Double, java_lang_Double_,
				value.getDouble());
		break;
	}
	default: {
		std::stringstream ss;
		ss << "Unknown scalar type " << value.getScalarType();
		env->ThrowNew(env->FindClass(LIB_EXCEPTION), ss.str().c_str());
		return env->NewGlobalRef(NULL);
	}
	}
	return scalar;
}

ParamId *Native2J::createParamId(JNIEnv *env, jint ns, jobject paramId) {
	ParamId *lparamId = NULL;
	jclass strClass = env->FindClass("java/lang/String");
	if (env->IsInstanceOf(paramId, strClass)) {
		const char *cstr = env->GetStringUTFChars((jstring) paramId, NULL);
		std::string *nodeId = new std::string(cstr);
		lparamId = new ParamId(ns, *nodeId, true);
		env->ReleaseStringUTFChars((jstring) paramId, cstr);
	} else {
		jclass integerClass = env->FindClass("java/lang/Integer");
		jmethodID getVal = env->GetMethodID(integerClass, "intValue", "()I");
		int i = env->CallIntMethod(paramId, getVal);
		lparamId = new ParamId(ns, i);
	}
	return lparamId;
}

// Override MessageHandler
void Native2J::notificationReceived(Message& msg) {
	JNIEnv *tmpEnv;
	jvm->AttachCurrentThread((void **) &tmpEnv, NULL);
	Notification& notification = (Notification&) msg;
	std::string sKey("");
	ValueChanged value = createVariant(tmpEnv, notification);
	callValueChanged(tmpEnv, value.getKey(), value.getValue());
	jvm->DetachCurrentThread();
}

void Native2J::eventReceived(Message& msg) {
	JNIEnv *tmpEnv;
	jvm->AttachCurrentThread((void **) &tmpEnv, NULL);
	Event& event = (Event&) msg;
	callMessageReceived(tmpEnv, createVariant(tmpEnv, event));
	jvm->DetachCurrentThread();
}

void Native2J::connectionStateChanged(int state) {
	if (serverId.length() > 0) {
		JNIEnv *tmpEnv;
		jvm->AttachCurrentThread((void **) &tmpEnv, NULL);
		switch (state) {
		case 0: //Disconnect
			callUsabilityChanged(tmpEnv, tmpEnv->NewStringUTF(serverId.c_str()),
					false);
			break;
		case 1: //Connect
			callUsabilityChanged(tmpEnv, tmpEnv->NewStringUTF(serverId.c_str()),
					true);
			break;
		case 2: //ConnectionWarningWatchdogTimeout
		case 3: //ConnectionErrorApiReconnect
		case 4: //ServerShutdown
			callUsabilityChanged(tmpEnv, tmpEnv->NewStringUTF(serverId.c_str()),
					false);
			break;
		default:
			break;
		}
		jvm->DetachCurrentThread();
	}
}

void Native2J::setServerId(std::string sId) {
	serverId = sId;
}

Exception* Native2J::getException(JNIEnv *env){
	Exception *exception = NULL;
	if (env->ExceptionCheck()){
		jthrowable ex = env->ExceptionOccurred();
		env->ExceptionClear();
		jclass clazz = env->GetObjectClass(ex);
		jmethodID getMessage = env->GetMethodID(clazz,
												"getMessage",
												"()Ljava/lang/String;");
		jstring message = (jstring)env->CallObjectMethod(ex, getMessage);
		const char *mstr = env->GetStringUTFChars(message, NULL);
		exception = new ExceptionDef(CommonNamespace::Exception, mstr);
		env->ReleaseStringUTFChars(message, mstr);

		jmethodID getCause = env->GetMethodID(clazz,
												"getCause",
												"()Ljava/lang/Throwable;");
		jthrowable cause = (jthrowable)env->CallObjectMethod(ex, getCause);
		if (cause != NULL){
			cause = (jthrowable)env->CallObjectMethod(cause, getCause);
		}

		if (cause != NULL){
			clazz = env->GetObjectClass(cause);
			jmethodID getStatusCode = env->GetMethodID(clazz,"getStatusCode","()I");
			int code = env->CallIntMethod(cause, getStatusCode);
			exception->setErrorCode(new unsigned long(code));
		}
	}
	return exception;
}



