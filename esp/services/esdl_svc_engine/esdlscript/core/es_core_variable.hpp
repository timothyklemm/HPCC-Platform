/*##############################################################################

    HPCC SYSTEMS software Copyright (C) 2019 HPCC Systems®.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
############################################################################## */

#ifndef _EsdlScriptCoreVariable_HPP_
#define _EsdlScriptCoreVariable_HPP_

#include "esp.hpp"
#include "es_core_trace.hpp"
#include "es_core_utility.hpp"
#include <limits>

namespace EsdlScript
{

enum VariableState
{
    VariableState_Undefined = -1, // variable instance is uninitialized
    VariableState_Declared,       // variable name has been declared and a default value assigned
    VariableState_Defined,        // variable name has been defined with a runtime value
};
extern const char* mapVariableState(VariableState state);
extern VariableState mapVariableState(const char* label);

using VariableFrame = uint8_t;
static const VariableFrame CURRENT_VARIABLE_FRAME = std::numeric_limits<VariableFrame>::max();
static const VariableFrame GLOBAL_VARIABLE_FRAME = 0;
static const VariableFrame SCRIPT_VARIABLE_FRAME = 1;
static const VariableFrame PHASE_VARIABLE_FRAME  = 2;

interface IVariable : extends IInterface
{
    virtual VariableState getState() const = 0;
    inline bool isDeclared() const { return getState() == VariableState_Declared; }
    inline bool isDefined() const { return getState() == VariableState_Defined; }

    virtual VariableFrame getFrame() const = 0;
    inline bool isGlobalScope() const { return getFrame() == GLOBAL_VARIABLE_FRAME; }
    inline bool isScriptScope() const { return getFrame() == SCRIPT_VARIABLE_FRAME; }
    inline bool isPhaseScope() const { return getFrame() == PHASE_VARIABLE_FRAME; }

    virtual const char* queryName() const = 0;
    virtual const char* queryValue() const = 0;
};

interface IVariableIterator : extends IIteratorOf<const IVariable> {};

interface IVariableListener
{
    virtual void applyVariableUpdate(const char* name, const char* value) = 0;
};

// Variables are maintained in a stack, where a variable defined in a stack frame is visible in all
// following frames but is not visible to an preceding stack frame. This interface deviates from
// the traditional stack pattern by allowing direct access to any stack frame, enabling variable
// visibility to be controlled regardless of when a variable can be defined.
interface IVariables : extends virtual IInterface, extends virtual IPersistent
{
    struct StFrame
    {
        StFrame(IVariables& stack) : m_stack(stack) { m_stack.pushVariablesFrame(); }
        ~StFrame() { m_stack.popVariablesFrame(); }

        IVariables& m_stack;
    };

    virtual IVariableListener* getVariableListener() const = 0;
    virtual void setVariableListener(IVariableListener* listener) = 0;

    // Create a new variable stack frame based on the current frame.
    // Return the stack depth of the new set
    virtual VariableFrame pushVariablesFrame() = 0;
    // Adds a new variable visible to stack frame 'atFrame' and all newer frames
    virtual bool addVariable(VariableState state, const char* name, const char* value, VariableFrame atFrame = CURRENT_VARIABLE_FRAME) = 0;
    inline bool declareVariable(const char* name, const char* value, VariableFrame atFrame = CURRENT_VARIABLE_FRAME) { return addVariable(VariableState_Declared, name, value, atFrame); }
    inline bool defineVariable(const char* name, const char* value, VariableFrame atFrame = CURRENT_VARIABLE_FRAME) { return addVariable(VariableState_Defined, name, value, atFrame); }

    // Returns true only if the variable name is defined in one of the indicated frames
    virtual bool isVariable(const char* name, VariableFrame maxFrame = CURRENT_VARIABLE_FRAME, VariableFrame minFrame = 0) const = 0;
    // Returns the value of the variable defined in on of the indicated frames
    virtual const char* queryVariable(const char* name, VariableFrame maxFrame = CURRENT_VARIABLE_FRAME, VariableFrame minFrame = 0) const = 0;
    // Removes the current variable stack frame
    virtual void popVariablesFrame();
    // Returns a collection of all variables defined in the indicated range of frames.
    virtual IVariableIterator* getVariables(VariableFrame maxFrame = CURRENT_VARIABLE_FRAME, VariableFrame minFrame = GLOBAL_VARIABLE_FRAME) const = 0;
};

DECLARE_IOWNER(Variables);

} // namespace EsdlScript

#endif // _EsdlScriptCoreVariable_HPP_
