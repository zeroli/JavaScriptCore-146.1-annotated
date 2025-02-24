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
#include <Foundation/Foundation.h>

#include <objc_class.h>
#include <objc_utility.h>
#include <WebScriptObject.h>

using namespace KJS::Bindings;

void ObjcClass::_commonDelete() {
    CFRelease (_fields);
    CFRelease (_methods);
}
    

void ObjcClass::_commonCopy(const ObjcClass &other) {
    _isa = other._isa;
    _methods = CFDictionaryCreateCopy (NULL, other._methods);
    _fields = CFDictionaryCreateCopy (NULL, other._fields);
}
    

void ObjcClass::_commonInit (ClassStructPtr aClass)
{
    _isa = aClass;
    _methods = CFDictionaryCreateMutable(NULL, 16, &kCFTypeDictionaryKeyCallBacks, NULL);
    _fields = CFDictionaryCreateMutable(NULL, 16, &kCFTypeDictionaryKeyCallBacks, NULL);
}


static CFMutableDictionaryRef classesByIsA = 0;

static void _createClassesByIsAIfNecessary()
{
    if (classesByIsA == 0)
        classesByIsA = CFDictionaryCreateMutable (NULL, 0, NULL, NULL);
}

ObjcClass *ObjcClass::classForIsA (ClassStructPtr isa)
{
    _createClassesByIsAIfNecessary();
    
    ObjcClass *aClass = (ObjcClass *)CFDictionaryGetValue(classesByIsA, isa);
    if (aClass == NULL) {
        aClass = new ObjcClass (isa);
        CFDictionaryAddValue (classesByIsA, isa, aClass);
    }
    
    return aClass;
}


ObjcClass::ObjcClass (ClassStructPtr isa)
{
    _commonInit (isa);
}

const char *ObjcClass::name() const
{
    return _isa->name;
}

MethodList ObjcClass::methodsNamed(const char *_name) const
{
    MethodList methodList;
    char name[4096];
    
    JSMethodNameToObjCMethodName (_name, name, 4096);
    
    if (*name == 0) {
        return methodList;
    }
        
    CFStringRef methodName = CFStringCreateWithCString(NULL, name, kCFStringEncodingASCII);
    Method *method = (Method *)CFDictionaryGetValue (_methods, methodName);
    if (method) {
        CFRelease (methodName);
        methodList.addMethod(method);
        return methodList;
    }
    
    ClassStructPtr thisClass = _isa;
    while (thisClass != 0 && methodList.length() < 1) {
        void *iterator = 0;
        struct objc_method_list *objcMethodList;
        while ( (objcMethodList = class_nextMethodList( thisClass, &iterator )) ) {
            int i, numMethodsInClass = objcMethodList->method_count;
            for (i = 0; i < numMethodsInClass; i++) {
                struct objc_method *objcMethod = &objcMethodList->method_list[i];
                NSString *mappedName = 0;
            
                // See if the class wants to exclude the selector from visibility in JavaScript.
                if ([(id)thisClass isSelectorExcludedFromWebScript:objcMethod->method_name]) {
                    continue;
                }
                
                // See if the class want to provide a different name for the selector in JavaScript.
                // Note that we do not do any checks to guarantee uniqueness. That's the responsiblity
                // of the class.
                if ([(id)thisClass respondsToSelector:@selector(webScriptNameForSelector:)]){
                    mappedName = [(id)thisClass webScriptNameForSelector: objcMethod->method_name];
                }

                if ((mappedName && [mappedName isEqual:(NSString *)methodName]) ||
                    strcmp ((const char *)objcMethod->method_name, name) == 0) {
                    Method *aMethod = new ObjcMethod (thisClass, (const char *)objcMethod->method_name);
                    CFDictionaryAddValue ((CFMutableDictionaryRef)_methods, methodName, aMethod);
                    methodList.addMethod (aMethod);
                    break;
                }
            }
        } 
        thisClass = thisClass->super_class;
    }

    CFRelease (methodName);
    
    return methodList;
}


Field *ObjcClass::fieldNamed(const char *name) const
{
    ClassStructPtr thisClass = _isa;

    CFStringRef fieldName = CFStringCreateWithCString(NULL, name, kCFStringEncodingASCII);
    Field *aField = (Field *)CFDictionaryGetValue (_fields, fieldName);
    if (aField) {
        CFRelease (fieldName);
        return aField;
    }

    while (thisClass != 0) {
        struct objc_ivar_list *fieldsInClass = thisClass->ivars;
        if (fieldsInClass) {
            int i, numFieldsInClass = fieldsInClass->ivar_count;
            for (i = 0; i < numFieldsInClass; i++) {
                Ivar objcIVar = &fieldsInClass->ivar_list[i];
                NSString *mappedName = 0;

                // See if the class wants to exclude the selector from visibility in JavaScript.
                if ([(id)thisClass isKeyExcludedFromWebScript:objcIVar->ivar_name]) {
                    continue;
                }
                
                // See if the class want to provide a different name for the selector in JavaScript.
                // Note that we do not do any checks to guarantee uniqueness. That's the responsiblity
                // of the class.
                if ([(id)thisClass respondsToSelector:@selector(webScriptNameForKey:)]){
                    mappedName = [(id)thisClass webScriptNameForKey:objcIVar->ivar_name];
                }

                if ((mappedName && [mappedName isEqual:(NSString *)fieldName]) ||
                    strcmp(objcIVar->ivar_name,name) == 0) {
                    aField = new ObjcField (objcIVar);
                    CFDictionaryAddValue ((CFMutableDictionaryRef)_fields, fieldName, aField);
                    break;
                }
            }
        }
        thisClass = thisClass->super_class;
    }

    CFRelease (fieldName);

    return aField;
};
