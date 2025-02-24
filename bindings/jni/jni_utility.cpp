/*
 * Copyright (C) 2003 Apple Computer, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */
#include <interpreter.h>
#include <list.h>

#include "jni_runtime.h"
#include "jni_utility.h"
#include "runtime_array.h"
#include "runtime_object.h"

using namespace KJS::Bindings;

static JavaVM *jvm = 0;

JavaVM *KJS::Bindings::getJavaVM()
{
    if (jvm)
        return jvm;

    JavaVM *jvmArray[1];
    jsize bufLen = 1;
    jsize nJVMs = 0;
    jint jniError = 0;

    // Assumes JVM is already running ..., one per process
    jniError = JNI_GetCreatedJavaVMs(jvmArray, bufLen, &nJVMs);
    if ( jniError == JNI_OK && nJVMs > 0 ) {
        jvm = jvmArray[0];
    }
    else 
        fprintf(stderr, "%s: JNI_GetCreatedJavaVMs failed, returned %d\n", __PRETTY_FUNCTION__, jniError);
        
    return jvm;
}

JNIEnv *KJS::Bindings::getJNIEnv()
{
    JNIEnv *env;
    jint jniError = 0;

    jniError = (getJavaVM())->AttachCurrentThread((void**)&env, (void *)NULL);
    if ( jniError == JNI_OK )
        return env;
    else
        fprintf(stderr, "%s: AttachCurrentThread failed, returned %d\n", __PRETTY_FUNCTION__, jniError);
    return NULL;
}

static jvalue callJNIMethod (JNIType type, jobject obj, const char *name, const char *sig, va_list args)
{
    JavaVM *jvm = getJavaVM();
    JNIEnv *env = getJNIEnv();
    jvalue result;

    bzero (&result, sizeof(jvalue));
    if ( obj != NULL && jvm != NULL && env != NULL) {
        jclass cls = env->GetObjectClass(obj);
        if ( cls != NULL ) {
            jmethodID mid = env->GetMethodID(cls, name, sig);
            if ( mid != NULL )
            {
                switch (type) {
                case void_type:
                    env->functions->CallVoidMethodV(env, obj, mid, args);
                    break;
                case object_type:
                    result.l = env->functions->CallObjectMethodV(env, obj, mid, args);
                    break;
                case boolean_type:
                    result.z = env->functions->CallBooleanMethodV(env, obj, mid, args);
                    break;
                case byte_type:
                    result.b = env->functions->CallByteMethodV(env, obj, mid, args);
                    break;
                case char_type:
                    result.c = env->functions->CallCharMethodV(env, obj, mid, args);
                    break;
                case short_type:
                    result.s = env->functions->CallShortMethodV(env, obj, mid, args);
                    break;
                case int_type:
                    result.i = env->functions->CallIntMethodV(env, obj, mid, args);
                    break;
                case long_type:
                    result.j = env->functions->CallLongMethodV(env, obj, mid, args);
                    break;
                case float_type:
                    result.f = env->functions->CallFloatMethodV(env, obj, mid, args);
                    break;
                case double_type:
                    result.d = env->functions->CallDoubleMethodV(env, obj, mid, args);
                    break;
                default:
                    fprintf(stderr, "%s: invalid function type (%d)\n", __PRETTY_FUNCTION__, (int)type);
                }
            }
            else
            {
                fprintf(stderr, "%s: Could not find method: %s for %p\n", __PRETTY_FUNCTION__, name, obj);
                env->ExceptionDescribe();
                env->ExceptionClear();
                fprintf (stderr, "\n");
            }

            env->DeleteLocalRef(cls);
        }
        else {
            fprintf(stderr, "%s: Could not find class for %p\n", __PRETTY_FUNCTION__, obj);
        }
    }

    return result;
}

static jvalue callJNIMethodIDA (JNIType type, jobject obj, jmethodID mid, jvalue *args)
{
    JNIEnv *env = getJNIEnv();
    jvalue result;
    
    bzero (&result, sizeof(jvalue));
    if ( obj != NULL && mid != NULL )
    {
        switch (type) {
        case void_type:
            env->functions->CallVoidMethodA(env, obj, mid, args);
            break;
        case object_type:
            result.l = env->functions->CallObjectMethodA(env, obj, mid, args);
            break;
        case boolean_type:
            result.z = env->functions->CallBooleanMethodA(env, obj, mid, args);
            break;
        case byte_type:
            result.b = env->functions->CallByteMethodA(env, obj, mid, args);
            break;
        case char_type:
            result.c = env->functions->CallCharMethodA(env, obj, mid, args);
            break;
        case short_type:
            result.s = env->functions->CallShortMethodA(env, obj, mid, args);
            break;
        case int_type:
            result.i = env->functions->CallIntMethodA(env, obj, mid, args);
            break;
        case long_type:
            result.j = env->functions->CallLongMethodA(env, obj, mid, args);
            break;
        case float_type:
            result.f = env->functions->CallFloatMethodA(env, obj, mid, args);
            break;
        case double_type:
            result.d = env->functions->CallDoubleMethodA(env, obj, mid, args);
            break;
        default:
            fprintf(stderr, "%s: invalid function type (%d)\n", __PRETTY_FUNCTION__, (int)type);
        }
    }

    return result;
}

static jvalue callJNIMethodA (JNIType type, jobject obj, const char *name, const char *sig, jvalue *args)
{
    JavaVM *jvm = getJavaVM();
    JNIEnv *env = getJNIEnv();
    jvalue result;
    
    bzero (&result, sizeof(jvalue));
    if ( obj != NULL && jvm != NULL && env != NULL) {
        jclass cls = env->GetObjectClass(obj);
        if ( cls != NULL ) {
            jmethodID mid = env->GetMethodID(cls, name, sig);
            if ( mid != NULL ) {
                result = callJNIMethodIDA (type, obj, mid, args);
            }
            else {
                fprintf(stderr, "%s: Could not find method: %s\n", __PRETTY_FUNCTION__, name);
                env->ExceptionDescribe();
                env->ExceptionClear();
                fprintf (stderr, "\n");
            }

            env->DeleteLocalRef(cls);
        }
        else {
            fprintf(stderr, "%s: Could not find class for object\n", __PRETTY_FUNCTION__);
        }
    }

    return result;
}

jmethodID KJS::Bindings::getMethodID (jobject obj, const char *name, const char *sig)
{
    JNIEnv *env = getJNIEnv();
    jmethodID mid = 0;
	
    if ( env != NULL) {
    jclass cls = env->GetObjectClass(obj);
    if ( cls != NULL ) {
            mid = env->GetMethodID(cls, name, sig);
        }
        env->DeleteLocalRef(cls);
    }
    return mid;
}


#define CALL_JNI_METHOD(function_type,obj,name,sig) \
    va_list args;\
    va_start (args, sig);\
    \
    jvalue result = callJNIMethod(function_type, obj, name, sig, args);\
    \
    va_end (args);

void KJS::Bindings::callJNIVoidMethod (jobject obj, const char *name, const char *sig, ... )
{
    CALL_JNI_METHOD (void_type, obj, name, sig);
}

jobject KJS::Bindings::callJNIObjectMethod (jobject obj, const char *name, const char *sig, ... )
{
    CALL_JNI_METHOD (object_type, obj, name, sig);
    return result.l;
}

jboolean KJS::Bindings::callJNIBooleanMethod( jobject obj, const char *name, const char *sig, ... )
{
    CALL_JNI_METHOD (boolean_type, obj, name, sig);
    return result.z;
}

jbyte KJS::Bindings::callJNIByteMethod( jobject obj, const char *name, const char *sig, ... )
{
    CALL_JNI_METHOD (byte_type, obj, name, sig);
    return result.b;
}

jchar KJS::Bindings::callJNICharMethod (jobject obj, const char *name, const char *sig, ... )
{
    CALL_JNI_METHOD (char_type, obj, name, sig);
    return result.c;
}

jshort KJS::Bindings::callJNIShortMethod (jobject obj, const char *name, const char *sig, ... )
{
    CALL_JNI_METHOD (short_type, obj, name, sig);
    return result.s;
}

jint KJS::Bindings::callJNIIntMethod (jobject obj, const char *name, const char *sig, ... )
{
    CALL_JNI_METHOD (int_type, obj, name, sig);
    return result.i;
}

jlong KJS::Bindings::callJNILongMethod (jobject obj, const char *name, const char *sig, ... )
{
    CALL_JNI_METHOD (long_type, obj, name, sig);
    return result.j;
}

jfloat KJS::Bindings::callJNIFloatMethod (jobject obj, const char *name, const char *sig, ... )
{
    CALL_JNI_METHOD (float_type, obj, name, sig);
    return result.f;
}

jdouble KJS::Bindings::callJNIDoubleMethod (jobject obj, const char *name, const char *sig, ... )
{
    CALL_JNI_METHOD (double_type, obj, name, sig);
    return result.d;
}

void KJS::Bindings::callJNIVoidMethodA (jobject obj, const char *name, const char *sig, jvalue *args)
{
    jvalue result = callJNIMethodA (void_type, obj, name, sig, args);
}

jobject KJS::Bindings::callJNIObjectMethodA (jobject obj, const char *name, const char *sig, jvalue *args)
{
    jvalue result = callJNIMethodA (object_type, obj, name, sig, args);
    return result.l;
}

jbyte KJS::Bindings::callJNIByteMethodA ( jobject obj, const char *name, const char *sig, jvalue *args)
{
    jvalue result = callJNIMethodA (byte_type, obj, name, sig, args);
    return result.b;
}

jchar KJS::Bindings::callJNICharMethodA (jobject obj, const char *name, const char *sig, jvalue *args)
{
    jvalue result = callJNIMethodA (char_type, obj, name, sig, args);
    return result.c;
}

jshort KJS::Bindings::callJNIShortMethodA (jobject obj, const char *name, const char *sig, jvalue *args)
{
    jvalue result = callJNIMethodA (short_type, obj, name, sig, args);
    return result.s;
}

jint KJS::Bindings::callJNIIntMethodA (jobject obj, const char *name, const char *sig, jvalue *args)
{
    jvalue result = callJNIMethodA (int_type, obj, name, sig, args);
    return result.i;
}

jlong KJS::Bindings::callJNILongMethodA (jobject obj, const char *name, const char *sig, jvalue *args)
{
    jvalue result = callJNIMethodA (long_type, obj, name, sig, args);
    return result.j;
}

jfloat KJS::Bindings::callJNIFloatMethodA (jobject obj, const char *name, const char *sig, jvalue *args)
{
    jvalue result = callJNIMethodA  (float_type, obj, name, sig, args);
    return result.f;
}

jdouble KJS::Bindings::callJNIDoubleMethodA (jobject obj, const char *name, const char *sig, jvalue *args)
{
    jvalue result = callJNIMethodA (double_type, obj, name, sig, args);
    return result.d;
}

jboolean KJS::Bindings::callJNIBooleanMethodA (jobject obj, const char *name, const char *sig, jvalue *args)
{
    jvalue result = callJNIMethodA (boolean_type, obj, name, sig, args);
    return result.z;
}

void KJS::Bindings::callJNIVoidMethodIDA (jobject obj, jmethodID methodID, jvalue *args)
{
    jvalue result = callJNIMethodIDA (void_type, obj, methodID, args);
}

jobject KJS::Bindings::callJNIObjectMethodIDA (jobject obj, jmethodID methodID, jvalue *args)
{
    jvalue result = callJNIMethodIDA (object_type, obj, methodID, args);
    return result.l;
}

jbyte KJS::Bindings::callJNIByteMethodIDA ( jobject obj, jmethodID methodID, jvalue *args)
{
    jvalue result = callJNIMethodIDA (byte_type, obj, methodID, args);
    return result.b;
}

jchar KJS::Bindings::callJNICharMethodIDA (jobject obj, jmethodID methodID, jvalue *args)
{
    jvalue result = callJNIMethodIDA (char_type, obj, methodID, args);
    return result.c;
}

jshort KJS::Bindings::callJNIShortMethodIDA (jobject obj, jmethodID methodID, jvalue *args)
{
    jvalue result = callJNIMethodIDA (short_type, obj, methodID, args);
    return result.s;
}

jint KJS::Bindings::callJNIIntMethodIDA (jobject obj, jmethodID methodID, jvalue *args)
{
    jvalue result = callJNIMethodIDA (int_type, obj, methodID, args);
    return result.i;
}

jlong KJS::Bindings::callJNILongMethodIDA (jobject obj, jmethodID methodID, jvalue *args)
{
    jvalue result = callJNIMethodIDA (long_type, obj, methodID, args);
    return result.j;
}

jfloat KJS::Bindings::callJNIFloatMethodIDA (jobject obj, jmethodID methodID, jvalue *args)
{
    jvalue result = callJNIMethodIDA  (float_type, obj, methodID, args);
    return result.f;
}

jdouble KJS::Bindings::callJNIDoubleMethodIDA (jobject obj, jmethodID methodID, jvalue *args)
{
    jvalue result = callJNIMethodIDA (double_type, obj, methodID, args);
    return result.d;
}

jboolean KJS::Bindings::callJNIBooleanMethodIDA (jobject obj, jmethodID methodID, jvalue *args)
{
    jvalue result = callJNIMethodIDA (boolean_type, obj, methodID, args);
    return result.z;
}

const char *KJS::Bindings::getCharactersFromJString (jstring aJString)
{
    return getCharactersFromJStringInEnv (getJNIEnv(), aJString);
}

void KJS::Bindings::releaseCharactersForJString (jstring aJString, const char *s)
{
    releaseCharactersForJStringInEnv (getJNIEnv(), aJString, s);
}

const char *KJS::Bindings::getCharactersFromJStringInEnv (JNIEnv *env, jstring aJString)
{
    jboolean isCopy;
    const char *s = env->GetStringUTFChars((jstring)aJString, &isCopy);
    if (!s) {
        env->ExceptionDescribe();
        env->ExceptionClear();
		fprintf (stderr, "\n");
    }
    return s;
}

void KJS::Bindings::releaseCharactersForJStringInEnv (JNIEnv *env, jstring aJString, const char *s)
{
    env->ReleaseStringUTFChars (aJString, s);
}

const jchar *KJS::Bindings::getUCharactersFromJStringInEnv (JNIEnv *env, jstring aJString)
{
    jboolean isCopy;
    const jchar *s = env->GetStringChars((jstring)aJString, &isCopy);
    if (!s) {
        env->ExceptionDescribe();
        env->ExceptionClear();
		fprintf (stderr, "\n");
    }
    return s;
}

void KJS::Bindings::releaseUCharactersForJStringInEnv (JNIEnv *env, jstring aJString, const jchar *s)
{
    env->ReleaseStringChars (aJString, s);
}

JNIType KJS::Bindings::JNITypeFromClassName(const char *name)
{
    JNIType type;
    
    if (strcmp("byte",name) == 0)
        type = byte_type;
    else if (strcmp("short",name) == 0)
        type = short_type;
    else if (strcmp("int",name) == 0)
        type = int_type;
    else if (strcmp("long",name) == 0)
        type = long_type;
    else if (strcmp("float",name) == 0)
        type = float_type;
    else if (strcmp("double",name) == 0)
        type = double_type;
    else if (strcmp("char",name) == 0)
        type = char_type;
    else if (strcmp("boolean",name) == 0)
        type = boolean_type;
    else if (strcmp("void",name) == 0)
        type = void_type;
    else
        type = object_type;
        
    return type;
}

const char *KJS::Bindings::signatureFromPrimitiveType(JNIType type)
{
    switch (type){
        case void_type: 
            return "V";
        
        case object_type:
            return "L";
        
        case boolean_type:
            return "Z";
        
        case byte_type:
            return "B";
            
        case char_type:
            return "C";
        
        case short_type:
            return "S";
        
        case int_type:
            return "I";
        
        case long_type:
            return "J";
        
        case float_type:
            return "F";
        
        case double_type:
            return "D";

        case invalid_type:
        default:
        break;
    }
    return "";
}

JNIType KJS::Bindings::JNITypeFromPrimitiveType(char type)
{
    switch (type){
        case 'V': 
            return void_type;
        
        case 'L':
        case '[':
            return object_type;
        
        case 'Z':
            return boolean_type;
        
        case 'B':
            return byte_type;
            
        case 'C':
            return char_type;
        
        case 'S':
            return short_type;
        
        case 'I':
            return int_type;
        
        case 'J':
            return long_type;
        
        case 'F':
            return float_type;
        
        case 'D':
            return double_type;

        default:
        break;
    }
    return invalid_type;
}

jvalue KJS::Bindings::getJNIField( jobject obj, JNIType type, const char *name, const char *signature)
{
    JavaVM *jvm = getJavaVM();
    JNIEnv *env = getJNIEnv();
    jvalue result;

    bzero (&result, sizeof(jvalue));
    if ( obj != NULL && jvm != NULL && env != NULL) {
        jclass cls = env->GetObjectClass(obj);
        if ( cls != NULL ) {
            jfieldID field = env->GetFieldID(cls, name, signature);
            if ( field != NULL ) {
                switch (type) {
                case object_type:
                    result.l = env->functions->GetObjectField(env, obj, field);
                    break;
                case boolean_type:
                    result.z = env->functions->GetBooleanField(env, obj, field);
                    break;
                case byte_type:
                    result.b = env->functions->GetByteField(env, obj, field);
                    break;
                case char_type:
                    result.c = env->functions->GetCharField(env, obj, field);
                    break;
                case short_type:
                    result.s = env->functions->GetShortField(env, obj, field);
                    break;
                case int_type:
                    result.i = env->functions->GetIntField(env, obj, field);
                    break;
                case long_type:
                    result.j = env->functions->GetLongField(env, obj, field);
                    break;
                case float_type:
                    result.f = env->functions->GetFloatField(env, obj, field);
                    break;
                case double_type:
                    result.d = env->functions->GetDoubleField(env, obj, field);
                    break;
                default:
                    fprintf(stderr, "%s: invalid field type (%d)\n", __PRETTY_FUNCTION__, (int)type);
                }
            }
            else
            {
                fprintf(stderr, "%s: Could not find field: %s\n", __PRETTY_FUNCTION__, name);
                env->ExceptionDescribe();
                env->ExceptionClear();
                fprintf (stderr, "\n");
            }

            env->DeleteLocalRef(cls);
        }
        else {
            fprintf(stderr, "%s: Could not find class for object\n", __PRETTY_FUNCTION__);
        }
    }

    return result;
}

jvalue KJS::Bindings::convertValueToJValue (KJS::ExecState *exec, KJS::Value value, JNIType _JNIType, const char *javaClassName)
{
    jvalue result;
   
    switch (_JNIType){
        case object_type: {
            result.l = (jobject)0;
            
            // First see if we have a Java instance.
            if (value.type() == KJS::ObjectType){
                KJS::ObjectImp *objectImp = static_cast<KJS::ObjectImp*>(value.imp());
                if (strcmp(objectImp->classInfo()->className, "RuntimeObject") == 0) {
                    KJS::RuntimeObjectImp *imp = static_cast<KJS::RuntimeObjectImp *>(value.imp());
                    JavaInstance *instance = static_cast<JavaInstance*>(imp->getInternalInstance());
                    result.l = instance->javaInstance();
                }
                else if (strcmp(objectImp->classInfo()->className, "RuntimeArray") == 0) {
                    KJS::RuntimeArrayImp *imp = static_cast<KJS::RuntimeArrayImp *>(value.imp());
                    JavaArray *array = static_cast<JavaArray*>(imp->getConcreteArray());
                    result.l = array->javaArray();
                }
            }
            
            // Now convert value to a string if the target type is a java.lang.string, and we're not
            // converting from a Null.
            if (result.l == 0 && strcmp(javaClassName, "java.lang.String") == 0 && value.type() != KJS::NullType) {
                KJS::UString stringValue = value.toString(exec);
                JNIEnv *env = getJNIEnv();
                jobject javaString = env->functions->NewString (env, (const jchar *)stringValue.data(), stringValue.size());
                result.l = javaString;
            }
        }
        break;
        
        case boolean_type: {
            result.z = (jboolean)value.toNumber(exec);
        }
        break;
            
        case byte_type: {
            result.b = (jbyte)value.toNumber(exec);
        }
        break;
        
        case char_type: {
            result.c = (jchar)value.toNumber(exec);
        }
        break;

        case short_type: {
            result.s = (jshort)value.toNumber(exec);
        }
        break;

        case int_type: {
            result.i = (jint)value.toNumber(exec);
        }
        break;

        case long_type: {
            result.j = (jlong)value.toNumber(exec);
        }
        break;

        case float_type: {
            result.f = (jfloat)value.toNumber(exec);
        }
        break;

        case double_type: {
            result.d = (jdouble)value.toNumber(exec);
        }
        break;
            
        break;

        case invalid_type:
        default:
        case void_type: {
            bzero (&result, sizeof(jvalue));
        }
        break;
    }
    return result;
}

