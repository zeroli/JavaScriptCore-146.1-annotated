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

#include "error_object.h"
#include "function.h"
#include "interpreter.h"
#include "object.h"
#include "operations.h"
#include "runtime_method.h"
#include "runtime_object.h"
#include "types.h"
#include "value.h"


#include <assert.h>

using namespace KJS;
using namespace Bindings;

const ClassInfo RuntimeObjectImp::info = {"RuntimeObject", 0, 0, 0};

RuntimeObjectImp::RuntimeObjectImp(ObjectImp *proto)
  : ObjectImp(proto)
{
    instance = 0;
}

RuntimeObjectImp::~RuntimeObjectImp()
{
    if (ownsInstance)
        delete instance;
}

RuntimeObjectImp::RuntimeObjectImp(Bindings::Instance *i, bool oi) : ObjectImp ((ObjectImp *)0)
{
    ownsInstance = oi;
    instance = i;
}

Value RuntimeObjectImp::get(ExecState *exec, const Identifier &propertyName) const
{
    instance->begin();
    
    Class *aClass = instance->getClass();
    
    if (aClass) {
        // See if the instance have a field with the specified name.
        Field *aField = aClass->fieldNamed(propertyName.ascii());
        if (aField) {
            return instance->getValueOfField (exec, aField); 
        }
        
        // Now check if a method with specified name exists, if so return a function object for
        // that method.
        MethodList methodList = aClass->methodsNamed(propertyName.ascii());
        if (methodList.length() > 0) {
            instance->end();
            return Object (new RuntimeMethodImp(exec, propertyName, methodList));
        }
    }
    
    instance->end();
    
    return Undefined();
}

void RuntimeObjectImp::put(ExecState *exec, const Identifier &propertyName,
                    const Value &value, int attr)
{
    instance->begin();

    // Set the value of the property.
    Field *aField = instance->getClass()->fieldNamed(propertyName.ascii());
    if (aField) {
        getInternalInstance()->setValueOfField(exec, aField, value);
    }

    instance->end();
}

bool RuntimeObjectImp::canPut(ExecState *exec, const Identifier &propertyName) const
{
    instance->begin();

    Field *aField = instance->getClass()->fieldNamed(propertyName.ascii());

    instance->end();

    return aField ? true : false;
}

bool RuntimeObjectImp::hasProperty(ExecState *exec,
                            const Identifier &propertyName) const
{
    instance->begin();

    Field *aField = instance->getClass()->fieldNamed(propertyName.ascii());
    if (aField) {
        instance->end();
        return true;
    }
        
    MethodList methodList = instance->getClass()->methodsNamed(propertyName.ascii());

    instance->end();

    if (methodList.length() > 0)
        return true;
        
    return false;
}

bool RuntimeObjectImp::deleteProperty(ExecState *exec,
                            const Identifier &propertyName)
{
    // Can never remove a property of a RuntimeObject.
    return false;
}

Value RuntimeObjectImp::defaultValue(ExecState *exec, Type hint) const
{
    instance->begin();

    Value aValue = getInternalInstance()->defaultValue(hint);
    
    instance->end();
    
    return aValue;
}
    
