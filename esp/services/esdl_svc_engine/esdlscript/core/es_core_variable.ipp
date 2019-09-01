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

#ifndef _EsdlScriptCoreVariable_IPP_
#define _EsdlScriptCoreVariable_IPP_

#include "es_core_variable.hpp"
#include "es_core_utility.ipp"
#include <set>
#include <map>

namespace EsdlScript
{

class CVariables : implements IVariables, extends CInterface, extends CPersistent
{
protected:
    using State = VariableState;
    using FrameIndex = VariableFrame;
    interface IMutableVariable : extends IVariable
    {
        virtual void updateState(State state) = 0;
        virtual void updateFrame(FrameIndex frame) = 0;
        virtual void updateValue(const char* value) = 0;
    };
    struct MutableVariableLess
    {
        bool operator () (const Owned<IMutableVariable>& lhs, const Owned<IMutableVariable>& rhs) const
        {
            if (lhs.get() == rhs.get())
                return false;
            if (lhs.get() == nullptr)
                return false;
            if (rhs.get() == nullptr)
                return true;
            if (lhs->getFrame() < rhs->getFrame())
                return true;
            if (lhs->getFrame() > rhs->getFrame())
                return false;
            if (lhs->queryName() == rhs->queryName())
                return false;
            if (lhs->queryName() == nullptr)
                return false;
            if (rhs->queryName() == nullptr)
                return false;
            return stricmp(lhs->queryName(), rhs->queryName()) < 0;
        }
    };
    using Frame = std::set<Owned<IMutableVariable>, MutableVariableLess>;
    using Stack = std::map<FrameIndex, Frame>;

    class CVariable : implements IMutableVariable, extends CInterface
    {
    public:
        CVariable(const char* name, const char* value, State state = VariableState_Defined, FrameIndex frame = CURRENT_VARIABLE_FRAME);
        ~CVariable();

        // IInterface
        IMPLEMENT_IINTERFACE;
        // IVariable
        State getState() const override { return m_state; }
        FrameIndex getFrame() const override { return m_frame; }
        const char* queryName() const override { return m_name; }
        const char* queryValue() const override { return m_value; }
        // IMutableVariable
        void updateState(State state) override;
        void updateFrame(FrameIndex frame) override;
        void updateValue(const char* value) override;

    protected:
        State m_state;
        FrameIndex m_frame;
        StringBuffer  m_name;
        StringBuffer  m_value;
    };

    class CVariableIterator : implements CInterfaceOf<const IVariableIterator>
    {
    public:
        CVariableIterator(const CVariables& variables, FrameIndex maxFrame, FrameIndex minFrame);
        ~CVariableIterator();
        bool first() override;
        bool next() override;
        bool isValid() override;
        const IVariable& query() override;
    protected:
        Linked<const CVariables> m_variables;
        FrameIndex m_maxFrame = CURRENT_VARIABLE_FRAME;
        FrameIndex m_minFrame = GLOBAL_VARIABLE_FRAME;
        Stack::const_reverse_iterator m_stackIt;
        Frame::const_iterator m_frameIt;
    };

public:
    CVariables();
    ~CVariables();

    // IInterface
    IMPLEMENT_IINTERFACE;
    // IPersistent
    IMPLEMENT_IPERSISTENT_COLLECTION("Variables", "frames");
    // IVariables
    IVariableListener* getVariableListener() const override;
    void setVariableListener(IVariableListener* listener) override;
    FrameIndex pushVariablesFrame() override;
    bool addVariable(State state, const char* name, const char* value, FrameIndex frame = CURRENT_VARIABLE_FRAME) override;
    bool isVariable(const char* name, FrameIndex maxFrame = CURRENT_VARIABLE_FRAME, FrameIndex minFrame = GLOBAL_VARIABLE_FRAME) const override;
    const char* queryVariable(const char* name, FrameIndex maxFrame = CURRENT_VARIABLE_FRAME, FrameIndex minFrame = GLOBAL_VARIABLE_FRAME) const override;
    void popVariablesFrame() override;
    IVariableIterator* getVariables(FrameIndex maxFrame = CURRENT_VARIABLE_FRAME, FrameIndex minFrame = GLOBAL_VARIABLE_FRAME) const override;

protected:
    virtual void applyVariableUpdate(const char* name, const char* value);

protected:
    virtual IMutableVariable* createVariable(State state, const char* name, const char* value, FrameIndex frame = CURRENT_VARIABLE_FRAME) const;
    inline IMutableVariable* createDeclaredVariable(const char* name, const char* value, FrameIndex frame = CURRENT_VARIABLE_FRAME) const { return createVariable(VariableState_Declared, name, value, frame); }
    inline IMutableVariable* createDefinedVariable(const char* name, const char* value, FrameIndex frame = CURRENT_VARIABLE_FRAME) const { return createVariable(VariableState_Defined, name, value, frame); }

protected:
    IVariableListener* m_listener = nullptr;
    Stack m_stack;
};

} // namespace EsdlScript

#endif // _EsdlScriptCoreVariable_IPP_
