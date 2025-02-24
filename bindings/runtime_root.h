/*
 * Copyright (C) 2004 Apple Computer, Inc.  All rights reserved.
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
#ifndef _RUNTIME_ROOT_H_
#define _RUNTIME_ROOT_H_

#include <JavaScriptCore/interpreter.h>
#include <JavaScriptCore/object.h>
#include <JavaScriptCore/jni_jsobject.h>

namespace KJS {

namespace Bindings {

class RootObject;

typedef RootObject *(*FindRootObjectForNativeHandleFunctionPtr)(void *);

extern CFMutableDictionaryRef findReferenceDictionary(ObjectImp *imp);
extern const Bindings::RootObject *rootForImp (ObjectImp *imp);
extern const Bindings::RootObject *KJS::Bindings::rootForInterpreter (KJS::Interpreter *interpreter);
extern void addNativeReference (const Bindings::RootObject *root, ObjectImp *imp);
extern void removeNativeReference (ObjectImp *imp);

class RootObject
{
friend class JSObject;
public:
    RootObject (const void *nativeHandle) : _nativeHandle(nativeHandle), _imp(0), _interpreter(0) {}
    ~RootObject (){
#if !USE_CONSERVATIVE_GC
        _imp->deref();
#endif
#if USE_CONSERVATIVE_GC | TEST_CONSERVATIVE_GC
	gcUnprotect(_imp);
#endif
    }
    
    void setRootObjectImp (KJS::ObjectImp *i) { 
        _imp = i;
#if !USE_CONSERVATIVE_GC
        _imp->ref();

#endif
#if USE_CONSERVATIVE_GC | TEST_CONSERVATIVE_GC
	gcProtect(_imp);
#endif
    }
    
    KJS::ObjectImp *rootObjectImp() const { return _imp; }
    
    void setInterpreter (KJS::Interpreter *i) { _interpreter = i; }
    KJS::Interpreter *interpreter() const { return _interpreter; }

    void removeAllNativeReferences ();


    // Must be called from the thread that will be used to access JavaScript.
    static void setFindRootObjectForNativeHandleFunction(FindRootObjectForNativeHandleFunctionPtr aFunc);
    static FindRootObjectForNativeHandleFunctionPtr findRootObjectForNativeHandleFunction() {
        return _findRootObjectForNativeHandleFunctionPtr;
    }
    
    static CFRunLoopRef runLoop() { return _runLoop; }
    static CFRunLoopSourceRef performJavaScriptSource() { return _performJavaScriptSource; }
    
    static void dispatchToJavaScriptThread(JSObjectCallContext *context);
    
private:
    const void *_nativeHandle;
    KJS::ObjectImp *_imp;
    KJS::Interpreter *_interpreter;

    static FindRootObjectForNativeHandleFunctionPtr _findRootObjectForNativeHandleFunctionPtr;
    static CFRunLoopRef _runLoop;
    static CFRunLoopSourceRef _performJavaScriptSource;
};

} // namespace Bindings

} // namespace KJS

#endif
