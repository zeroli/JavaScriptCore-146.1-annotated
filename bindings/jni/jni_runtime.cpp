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
#include <internal.h>
#include <ustring.h>
#include <value.h>

#include <jni_utility.h>
#include <jni_runtime.h>

#include <runtime_array.h>
#include <runtime_object.h>

using namespace KJS;
using namespace KJS::Bindings;


JavaParameter::JavaParameter (JNIEnv *env, jstring type)
{
    _type = JavaString (env, type);
    _JNIType = JNITypeFromClassName (_type.UTF8String());
};

JavaField::JavaField (JNIEnv *env, jobject aField)
{
    // Get field type
    jobject fieldType = callJNIObjectMethod (aField, "getType", "()Ljava/lang/Class;");
    jstring fieldTypeName = (jstring)callJNIObjectMethod (fieldType, "getName", "()Ljava/lang/String;");
    _type = JavaString(env, fieldTypeName);
    _JNIType = JNITypeFromClassName (_type.UTF8String());

    // Get field name
    jstring fieldName = (jstring)callJNIObjectMethod (aField, "getName", "()Ljava/lang/String;");
    _name = JavaString(env, fieldName);

    _field = new JavaInstance(aField);
}

KJS::Value JavaArray::convertJObjectToArray (KJS::ExecState *exec, jobject anObject, const char *type)
{
    if (type[0] != '[')
        return Undefined();

    return KJS::Object(new RuntimeArrayImp(exec, new JavaArray ((jobject)anObject, type)));
}

KJS::Value JavaField::valueFromInstance(KJS::ExecState *exec, const Instance *i) const 
{
    const JavaInstance *instance = static_cast<const JavaInstance *>(i);
    jobject jinstance = instance->javaInstance();
    jobject fieldJInstance = _field->javaInstance();

    switch (_JNIType) {
        case object_type: {
            jobject anObject = callJNIObjectMethod(_field->javaInstance(), "get", "(Ljava/lang/Object;)Ljava/lang/Object;", jinstance);

            const char *arrayType = type();
            if (arrayType[0] == '[') {
                return JavaArray::convertJObjectToArray (0, anObject, arrayType);
            }
            else {
                return KJS::Object(new RuntimeObjectImp(new JavaInstance ((jobject)anObject)));
            }
        }
        break;
            
        case boolean_type: {
            jboolean value = callJNIBooleanMethod(fieldJInstance, "getBoolean", "(Ljava/lang/Object;)Z", jinstance);
            return KJS::Boolean((bool)value);
        }
        break;
            
        case byte_type:
        case char_type:
        case short_type:
        
        case int_type:
            jint value;
            value = callJNIIntMethod(fieldJInstance, "getInt", "(Ljava/lang/Object;)I", jinstance);
            return Number((int)value);

        case long_type:
        case float_type:
        case double_type: {
            jdouble value;
            value = callJNIDoubleMethod(fieldJInstance, "getDouble", "(Ljava/lang/Object;)D", jinstance);
            return Number((double)value);
        }
        break;
        default:
        break;
    }
    return Undefined();
}

void JavaField::setValueToInstance(KJS::ExecState *exec, const Instance *i, const KJS::Value &aValue) const
{
    const JavaInstance *instance = static_cast<const JavaInstance *>(i);
    jobject jinstance = instance->javaInstance();
    jobject fieldJInstance = _field->javaInstance();
    jvalue javaValue = convertValueToJValue (exec, aValue, _JNIType, type());

    switch (_JNIType) {
        case object_type: {
            callJNIVoidMethod(fieldJInstance, "set", "(Ljava/lang/Object;Ljava/lang/Object;)V", jinstance, javaValue.l);
        }
        break;
            
        case boolean_type: {
            callJNIVoidMethod(fieldJInstance, "setBoolean", "(Ljava/lang/Object;Z)V", jinstance, javaValue.z);
        }
        break;
            
        case byte_type: {
            callJNIVoidMethod(fieldJInstance, "setByte", "(Ljava/lang/Object;B)V", jinstance, javaValue.b);
        }
        break;

        case char_type: {
            callJNIVoidMethod(fieldJInstance, "setChar", "(Ljava/lang/Object;C)V", jinstance, javaValue.c);
        }
        break;

        case short_type: {
            callJNIVoidMethod(fieldJInstance, "setShort", "(Ljava/lang/Object;S)V", jinstance, javaValue.s);
        }
        break;

        case int_type: {
            callJNIVoidMethod(fieldJInstance, "setInt", "(Ljava/lang/Object;I)V", jinstance, javaValue.i);
        }
        break;

        case long_type: {
            callJNIVoidMethod(fieldJInstance, "setLong", "(Ljava/lang/Object;J)V", jinstance, javaValue.j);
        }
        break;

        case float_type: {
            callJNIVoidMethod(fieldJInstance, "setFloat", "(Ljava/lang/Object;F)V", jinstance, javaValue.f);
        }
        break;

        case double_type: {
            callJNIVoidMethod(fieldJInstance, "setDouble", "(Ljava/lang/Object;D)V", jinstance, javaValue.d);
        }
        break;
        default:
        break;
    }
}

JavaConstructor::JavaConstructor (JNIEnv *env, jobject aConstructor)
{
    // Get parameters
    jarray jparameters = (jarray)callJNIObjectMethod (aConstructor, "getParameterTypes", "()[Ljava/lang/Class;");
    _numParameters = env->GetArrayLength (jparameters);
    _parameters = new JavaParameter[_numParameters];
    
    int i;
    for (i = 0; i < _numParameters; i++) {
        jobject aParameter = env->GetObjectArrayElement ((jobjectArray)jparameters, i);
        jstring parameterName = (jstring)callJNIObjectMethod (aParameter, "getName", "()Ljava/lang/String;");
        _parameters[i] = JavaParameter(env, parameterName);
        env->DeleteLocalRef (aParameter);
        env->DeleteLocalRef (parameterName);
    }
}

JavaMethod::JavaMethod (JNIEnv *env, jobject aMethod)
{
    // Get return type
    jobject returnType = callJNIObjectMethod (aMethod, "getReturnType", "()Ljava/lang/Class;");
    jstring returnTypeName = (jstring)callJNIObjectMethod (returnType, "getName", "()Ljava/lang/String;");
    _returnType =JavaString (env, returnTypeName);
    _JNIReturnType = JNITypeFromClassName (_returnType.UTF8String());
    env->DeleteLocalRef (returnType);
    env->DeleteLocalRef (returnTypeName);

    // Get method name
    jstring methodName = (jstring)callJNIObjectMethod (aMethod, "getName", "()Ljava/lang/String;");
    _name = JavaString (env, methodName);
    env->DeleteLocalRef (methodName);

    // Get parameters
    jarray jparameters = (jarray)callJNIObjectMethod (aMethod, "getParameterTypes", "()[Ljava/lang/Class;");
    _numParameters = env->GetArrayLength (jparameters);
    _parameters = new JavaParameter[_numParameters];
    
    int i;
    for (i = 0; i < _numParameters; i++) {
        jobject aParameter = env->GetObjectArrayElement ((jobjectArray)jparameters, i);
        jstring parameterName = (jstring)callJNIObjectMethod (aParameter, "getName", "()Ljava/lang/String;");
        _parameters[i] = JavaParameter(env, parameterName);
        env->DeleteLocalRef (aParameter);
        env->DeleteLocalRef (parameterName);
    }
    env->DeleteLocalRef (jparameters);

    // Created lazily.
    _signature = 0;
    _methodID = 0;
}

// JNI method signatures use '/' between components of a class name, but
// we get '.' between components from the reflection API.
static void appendClassName (UString *aString, const char *className)
{
    char *result, *cp = strdup(className);
    
    result = cp;
    while (*cp) {
        if (*cp == '.')
            *cp = '/';
        cp++;
    }
        
    aString->append(result);

    free (result);
}

const char *JavaMethod::signature() const 
{
    if (_signature == 0){
        int i;
        
        _signature = new UString("(");
        for (i = 0; i < _numParameters; i++) {
            JavaParameter *aParameter = static_cast<JavaParameter *>(parameterAt(i));
            JNIType _JNIType = aParameter->getJNIType();
            _signature->append(signatureFromPrimitiveType (_JNIType));
            if (_JNIType == object_type) {
                appendClassName (_signature, aParameter->type());
                _signature->append(";");
            }
        }
        _signature->append(")");
        
        const char *returnType = _returnType.UTF8String();
        if (returnType[0] == '[') {
            appendClassName (_signature, returnType);
        }
        else {
            _signature->append(signatureFromPrimitiveType (_JNIReturnType));
            if (_JNIReturnType == object_type) {
                appendClassName (_signature, returnType);
                _signature->append(";");
            }
        }
    }
    
    return _signature->ascii();
}

JNIType JavaMethod::JNIReturnType() const
{
    return _JNIReturnType;
}

jmethodID JavaMethod::methodID (jobject obj) const
{
    if (_methodID == 0) {
        _methodID = getMethodID (obj, name(), signature());
    }
    return _methodID;
}


JavaArray::JavaArray (jobject a, const char *t) 
{
    _array = new JObjectWrapper (a);
    // Java array are fixed length, so we can cache length.
    JNIEnv *env = getJNIEnv();
    _length = env->GetArrayLength((jarray)_array->_instance);

    _type = strdup(t);
};

JavaArray::~JavaArray () 
{
    _array->deref();
    free ((void *)_type);
}


JavaArray::JavaArray (const JavaArray &other) : Array() 
{
    _array = other._array;
    _array->ref();
    _type = strdup(other._type);
};

void JavaArray::setValueAt(KJS::ExecState *exec, unsigned int index, const KJS::Value &aValue) const
{
    JNIEnv *env = getJNIEnv();
    char *javaClassName = 0;
    
    JNIType arrayType = JNITypeFromPrimitiveType(_type[1]);
    if (_type[1] == 'L'){
        // The type of the array will be something like:
        // "[Ljava.lang.string;".  This is guaranteed, so no need
        // for extra sanity checks.
        javaClassName = strdup(&_type[2]);
        javaClassName[strchr(javaClassName, ';')-javaClassName] = 0;
    }
    jvalue aJValue = convertValueToJValue (exec, aValue, arrayType, javaClassName);
    
    switch (arrayType) {
        case object_type: {
            env->SetObjectArrayElement((jobjectArray)javaArray(), index, aJValue.l);
            break;
        }
            
        case boolean_type: {
            env->SetBooleanArrayRegion((jbooleanArray)javaArray(), index, 1, &aJValue.z);
            break;
        }
            
        case byte_type: {
            env->SetByteArrayRegion((jbyteArray)javaArray(), index, 1, &aJValue.b);
            break;
        }
            
        case char_type: {
            env->SetCharArrayRegion((jcharArray)javaArray(), index, 1, &aJValue.c);
            break;
        }
            
        case short_type: {
            env->SetShortArrayRegion((jshortArray)javaArray(), index, 1, &aJValue.s);
            break;
        }
            
        case int_type: {
            env->SetIntArrayRegion((jintArray)javaArray(), index, 1, &aJValue.i);
            break;
        }
            
        case long_type: {
            env->SetLongArrayRegion((jlongArray)javaArray(), index, 1, &aJValue.j);
        }
            
        case float_type: {
            env->SetFloatArrayRegion((jfloatArray)javaArray(), index, 1, &aJValue.f);
            break;
        }
            
        case double_type: {
            env->SetDoubleArrayRegion((jdoubleArray)javaArray(), index, 1, &aJValue.d);
            break;
        }
        default:
        break;
    }
    
    if (javaClassName)
        free ((void *)javaClassName);
}


KJS::Value JavaArray::valueAt(KJS::ExecState *exec, unsigned int index) const
{
    JNIEnv *env = getJNIEnv();
    JNIType arrayType = JNITypeFromPrimitiveType(_type[1]);
    switch (arrayType) {
        case object_type: {
            jobjectArray objectArray = (jobjectArray)javaArray();
            jobject anObject;
            anObject = env->GetObjectArrayElement(objectArray, index);

            // Nested array?
            if (_type[1] == '[') {
                return JavaArray::convertJObjectToArray (0, anObject, _type+1);
            }
            // or array of other object type?
            return KJS::Object(new RuntimeObjectImp(new JavaInstance ((jobject)anObject)));
        }
            
        case boolean_type: {
            jbooleanArray booleanArray = (jbooleanArray)javaArray();
            jboolean aBoolean;
            env->GetBooleanArrayRegion(booleanArray, index, 1, &aBoolean);
            return KJS::Boolean (aBoolean);
        }
            
        case byte_type: {
            jbyteArray byteArray = (jbyteArray)javaArray();
            jbyte aByte;
            env->GetByteArrayRegion(byteArray, index, 1, &aByte);
            return Number (aByte);
        }
            
        case char_type: {
            jcharArray charArray = (jcharArray)javaArray();
            jchar aChar;
            env->GetCharArrayRegion(charArray, index, 1, &aChar);
            return Number (aChar);
            break;
        }
            
        case short_type: {
            jshortArray shortArray = (jshortArray)javaArray();
            jshort aShort;
            env->GetShortArrayRegion(shortArray, index, 1, &aShort);
            return Number (aShort);
        }
            
        case int_type: {
            jintArray intArray = (jintArray)javaArray();
            jint anInt;
            env->GetIntArrayRegion(intArray, index, 1, &anInt);
            return Number (anInt);
        }
            
        case long_type: {
            jlongArray longArray = (jlongArray)javaArray();
            jlong aLong;
            env->GetLongArrayRegion(longArray, index, 1, &aLong);
            return Number ((long int)aLong);
        }
            
        case float_type: {
            jfloatArray floatArray = (jfloatArray)javaArray();
            jfloat aFloat;
            env->GetFloatArrayRegion(floatArray, index, 1, &aFloat);
            return Number (aFloat);
        }
            
        case double_type: {
            jdoubleArray doubleArray = (jdoubleArray)javaArray();
            jdouble aDouble;
            env->GetDoubleArrayRegion(doubleArray, index, 1, &aDouble);
            return Number (aDouble);
        }
        default:
        break;
    }
    return Undefined();
}

unsigned int JavaArray::getLength() const
{
    return _length;
}


