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

#include "es_core_variable.ipp"
#include "es_core_trace.hpp"
#include <algorithm>
#include <limits>

namespace EsdlScript
{

CVariables::CVariable::CVariable(const char* name, const char* value, State state, FrameIndex frame)
    : m_name(name)
    , m_value(value)
    , m_state(state)
    , m_frame(frame)
{
}

CVariables::CVariable::~CVariable()
{
}

void CVariables::CVariable::updateState(State state)
{
    if (m_state < state)
        m_state = state;
}

void CVariables::CVariable::updateFrame(FrameIndex frame)
{
    m_frame = frame;
}

void CVariables::CVariable::updateValue(const char* value)
{
    switch (m_state)
    {
    case VariableState_Undefined:
    case VariableState_Declared:
        m_value.set(value);
        break;
    case VariableState_Defined:
    default:
        break;
    }
}

CVariables::CVariableIterator::CVariableIterator(const CVariables& variables, FrameIndex maxFrame, FrameIndex minFrame)
    : m_variables(&variables)
    , m_maxFrame(maxFrame)
    , m_minFrame(minFrame)
{
    if (m_variables->m_stack.size())
    {
        if (CURRENT_VARIABLE_FRAME == m_minFrame && m_variables->m_stack.size())
            m_minFrame = m_variables->m_stack.begin()->first;
        if (CURRENT_VARIABLE_FRAME == m_maxFrame)
            m_maxFrame = m_variables->m_stack.rbegin()->first;
    }
}

CVariables::CVariableIterator::~CVariableIterator()
{
}

bool CVariables::CVariableIterator::first()
{
    m_stackIt = m_variables->m_stack.rbegin();
    while (m_stackIt != m_variables->m_stack.rend() && m_stackIt->first > m_maxFrame)
        ++m_stackIt;
    if (m_variables->m_stack.rend() == m_stackIt)
        return false;
    if (m_stackIt->first < m_minFrame)
    {
        m_stackIt = m_variables->m_stack.rend();
        return false;
    }
    m_frameIt = m_stackIt->second.begin();
    if (m_frameIt != m_stackIt->second.end())
        return true;
    else
        return next();
}

bool CVariables::CVariableIterator::next()
{
    if (m_variables->m_stack.rend() == m_stackIt)
        return false;
    if (m_frameIt != m_stackIt->second.end())
        ++m_frameIt;
    if (m_stackIt->second.end() == m_frameIt)
    {
        while (--m_stackIt != m_variables->m_stack.rend())
        {
            if (m_stackIt->first < m_minFrame)
            {
                m_stackIt = m_variables->m_stack.rend();
                return false;
            }
            m_frameIt = m_stackIt->second.begin();
            if (m_frameIt != m_stackIt->second.end())
                return true;
        }
    }
    return true;
}

bool CVariables::CVariableIterator::isValid()
{
    return (m_stackIt != m_variables->m_stack.rend() && m_frameIt != m_stackIt->second.end());
}

const IVariable& CVariables::CVariableIterator::query()
{
    if (!isValid())
        throw MakeStringException(0, "CVariableIterator::query called in invalid state");
    const IVariable* entry = m_frameIt->get();
    if (nullptr == entry)
        throw MakeStringException(0, "CVariableIterator::query encountered NULL entry");
    return *entry;
}

CVariables::CVariables()
{
    pushVariablesFrame();
}

CVariables::~CVariables()
{
}

static const char* variableStateString(VariableState state)
{
    switch (state)
    {
    case VariableState_Undefined: return "undefined";
    case VariableState_Declared: return "declared";
    case VariableState_Defined: return "defined";
    default: return "N/A";
    }
}

static const char* variableFrameString(VariableFrame frame)
{
    switch (frame)
    {
    case GLOBAL_VARIABLE_FRAME: return "global";
    case SCRIPT_VARIABLE_FRAME: return "script";
    case PHASE_VARIABLE_FRAME: return "phase";
    default: return "block";
    }
}

StringBuffer& CVariables::persist(StringBuffer& xml) const
{
    xml.appendf("<Variables frames=\"%zu\">", m_stack.size());
    for (auto& entry : m_stack)
    {
        const Frame& f = entry.second;

        xml.appendf("<Frame index=\"%u\" variables=\"%zu\" indexText=\"%s\">", unsigned(entry.first), f.size(), variableFrameString(entry.first));
        for (const Owned<IMutableVariable>& v : f)
        {
            xml << "<Variable "
                << " name=\"" << v->queryName() << "\""
                << " value=\"" << v->queryValue() << "\""
                << " state=\"" << v->getState() << "\""
                << " stateText=\"" << variableStateString(v->getState()) << "\""
                << "/>";
        }
        xml.append("</Frame>");
    }
    xml.append("</Variables>");

    return xml;
}

void CVariables::restoreInstance(IXMLParser& parser, const char* tag, size_t frameCount)
{
    FrameIndex frameIndex = 0;
    m_stack.clear();
    while (parser.next() && !parser.atEndTag(tag))
    {
        if (parser.atStartTag("Frame"))
        {
            Frame frame;
            FrameIndex index;

            restoring(parser.currentTag(), ++frameIndex, frameCount);
            if (!parser.currentAttribute("index", index))
            {
                Trace::internalError(0, "variable restoration did not find Variables/Frame/@index");
                parser.skip();
                break;
            }
            if (m_stack.count(index) != 0)
            {
                Trace::internalError(0, "variable restoration found duplicate frame %u", unsigned(index));
                parser.skip();
                break;
            }
            size_t variableIndex = 0, variableCount = 0;
            parser.currentAttribute("variables", variableCount);
            do
            {
                if (!parser.next())
                {
                    // error will be recorded in outer loop
                    break;
                }
                if (parser.atStartTag("Variable"))
                {
                    const char* name = parser.currentAttribute("name");
                    const char* value = parser.currentAttribute("value");
                    VariableState state;
                    bool good = true;

                    restoring(parser.currentTag(), ++variableIndex, variableCount);
                    if (isEmptyString(name))
                    {
                        Trace::internalError(0, "variable restoration found invalid name '%s'", name);
                        good = false;
                    }
                    if (std::find_if(frame.begin(), frame.end(), [name](const Owned<IMutableVariable>& entry) { return strieq(entry->queryName(), name); }) != frame.end())
                    {
                        Trace::internalError(0, "variable restoration found duplicate name '%s'", name);
                        good = false;
                    }
                    if (nullptr == value)
                    {
                        Trace::internalError(0, "variable restoration found NULL value");
                        good = false;
                    }
                    if (!parser.currentAttribute("state", state, VariableState_Undefined))
                    {
                        Trace::internalError(0, "variable restoration found invalid state '%s'", parser.currentAttribute("state"));
                        good = false;
                    }
                    if (state != VariableState_Declared && state != VariableState_Defined)
                    {
                        Trace::internalError(0, "variable restoration found invalid state '%d'", int(state));
                        good = false;
                    }
                    if (good)
                    {
                        frame.insert(new CVariable(name, value, state, index));
                    }
                }
                else if (parser.atStartTag())
                {
                    parser.skip();
                }
            }
            while (!parser.atEndTag("Frame"));
            m_stack[index] = frame;
        }
        else if (parser.atStartTag())
        {
            parser.skip();
        }
    }
}

IVariableListener* CVariables::getVariableListener() const
{
    return m_listener;
}

void CVariables::setVariableListener(IVariableListener* listener)
{
    m_listener = listener;
}

VariableFrame CVariables::pushVariablesFrame()
{
    FrameIndex index = (m_stack.empty() ? GLOBAL_VARIABLE_FRAME : m_stack.rbegin()->first + 1);
    m_stack[index];
    return index;
}

bool CVariables::addVariable(VariableState state, const char* name, const char* value, FrameIndex atFrame)
{
    if (CURRENT_VARIABLE_FRAME == atFrame)
        atFrame = m_stack.size() - 1;

    if (atFrame >= m_stack.size())
        return false;
    if (state != VariableState_Declared && state != VariableState_Defined)
        return false;
    if (isEmptyString(name) or isEmptyString(value))
        return false;

    auto& frame = m_stack[atFrame];
    Owned<IMutableVariable> needle(createVariable(state, name, value, atFrame));
    auto it = frame.find(needle);

    if (it != frame.end())
    {
        auto& match = *it;
        if (match->isDeclared() && needle->isDefined())
        {
            // A declared variable can always transition to defined.
            match->updateState(state);
            if (stricmp(match->queryValue(), needle->queryValue()) != 0)
            {
                match->updateValue(needle->queryValue());
                applyVariableUpdate(name, queryVariable(name, CURRENT_VARIABLE_FRAME, atFrame));
            }
        }
        else if (match->isDefined() && needle->isDeclared())
        {
            // A defined variable can be redeclared without error.
        }
        else if (match->isDeclared() && needle->isDeclared())
        {
            // Should re-declaration be allowed?
        }
        else if (match->isDefined() && needle->isDefined())
        {
            // Variable re-definition is not allowed;
            return false;
        }
        else
        {
            // Unexpected scenario
            return false;
        }
    }
    else
    {
        // new variable is always allowed
        auto newerValue = (atFrame == m_stack.size() - 1 ? nullptr : queryVariable(name, CURRENT_VARIABLE_FRAME, atFrame + 1));
        auto olderValue = (nullptr == newerValue && atFrame > 0 ? queryVariable(name, atFrame - 1) : nullptr);
        auto currentValue = (newerValue != nullptr ? newerValue : olderValue);

        frame.insert(needle);
        if (nullptr == currentValue || stricmp(value, currentValue) != 0)
        {
            applyVariableUpdate(name, value);
        }
    }

    return true;
}

bool CVariables::isVariable(const char* name, FrameIndex maxFrame, FrameIndex minFrame) const
{
    return queryVariable(name, maxFrame, minFrame) != nullptr;
}

const char* CVariables::queryVariable(const char* name, FrameIndex maxFrame, FrameIndex minFrame) const
{
    if (CURRENT_VARIABLE_FRAME == maxFrame)
        maxFrame = m_stack.size() - 1;
    if (CURRENT_VARIABLE_FRAME == minFrame)
        minFrame = m_stack.size() -1;

    if (minFrame > maxFrame || maxFrame >= m_stack.size())
        return nullptr;

    Owned<IMutableVariable> needle(createVariable(VariableState_Undefined, name, "."));

    for (FrameIndex idx = maxFrame; idx >= minFrame; idx--)
    {
        Stack::const_iterator stackIt = m_stack.find(idx);
        if (stackIt != m_stack.end())
        {
            needle->updateFrame(idx);

            auto  frameIt = stackIt->second.find(needle);

            if (frameIt != stackIt->second.end())
                return (*frameIt)->queryValue();
        }
    }

    return nullptr;
}

void CVariables::popVariablesFrame()
{
    if (m_stack.size() <= 1)
        throw MakeStringException(-1, "EsdlScript variable stack error [cannot pop base frame]");

    Stack::iterator it = std::prev(m_stack.end());
    Frame& frame = it->second;

    for (auto& v : frame)
        applyVariableUpdate(v->queryName(), queryVariable(v->queryName(), v->getFrame() - 1));
    m_stack.erase(it);
}

IVariableIterator* CVariables::getVariables(FrameIndex maxFrame, FrameIndex minFrame) const
{
    return new CVariableIterator(*this, maxFrame, minFrame);
}

void CVariables::applyVariableUpdate(const char* name, const char* value)
{
    if (m_listener != nullptr)
        m_listener->applyVariableUpdate(name, value);
}

CVariables::IMutableVariable* CVariables::createVariable(VariableState state, const char* name, const char* value, FrameIndex frame) const
{
    return new CVariable(name, value, state, frame);
}

} // namespace EsdlScript
