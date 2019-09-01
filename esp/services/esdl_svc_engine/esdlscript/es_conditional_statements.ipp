/*##############################################################################

    HPCC SYSTEMS software Copyright (C) 2013 HPCC Systems®.

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

#ifndef _EsConditionalAtatements_IPP__
#define _EsConditionalAtatements_IPP__

#include "esdl_script_core.ipp"

class CEvaluableStatement : public CStatement
{
public:
    CEvaluableStatement(const char* testAttribute = testAttribute);
    ~CEvaluableStatement()

    bool isEvaluable() const override;
    bool evaluate(IProcessContext* context, IParentContext* parentInfo) const override;

protected:
    bool initializeSelf(ILoadContext& context) override;

protected:
    StringBuffer m_testAttribute;
    Owned<ICompiledXpath> m_compiledXPath;
};

class CComparableStatement : public CEvaluableStatement
{
public:
    using CEvaluableStatement::CEvaluableStatement;

    bool evaluate(IProcessContext* context, IParentContext* parentInfo) const override;
};

class CConditionalDefaultStatement : public CStatement
{
public:
    CConditionalDefaultStatement();
    ~CConditionalDefaultStatement();

    bool isEvaluable() const override;
    bool evaluate() const override;
};

template <const char* SelfTag, const char* EvaluableTag, const char* DefaultTag>
class TConditionalSelectorStatement : public CStatement
{
    using Self = TConditionalSelectorStatement<SelfTag, EvaluableTag, DefaultTag>;
public:
    TConditionalSelectorStatement()
    {
        setChildAcceptor(acceptableChild);
        setChildPredicate(childSelector);
    }
    ~TConditionalSelectorStatement() {}

protected:
    void acceptChild(IStatement* child) override
    {
        auto defaultChild = dynamic_cast<CConditionalDefaultStatement*>(child);

        if (defaultChild != nullptr)
        {
            if (nullptr == m_defaultStatement)
            {
                m_defaultStatement = defaultChild;
                m_defaultPosition = m_children.insert(m_children.end(), child);
            }
            else
            {
                // throw on error, or assume caller validated call before calling?
            }
        }
        else if (m_defaultStatement != nullptr)
        {
            m_children.insert(m_defaultPosition, child);
        }
        else
        {
            m_children.push_back(child);
        }
    }
    static bool acceptableParent(const CStatement*, const IStatement* parent)
    {
        return (parent != nullptr && streq(parent->queryTag(), SelfTag));
    }

private:
    static bool acceptableChild(const CStatement* parent, const char* tag)
    {
        if (isEmptyString(tag))
            return false;
        if (streq(tag, EvaluableTag))
            return true;
        if (streq(tag, DefaultTag))
        {
            auto self = dynamic_cast<Self*>(parent);
            return (self != nullptr && nullptr == self->m_defaultStatement);
        }
        return false;
    }

    static uint8_t childSelector(const Owned<IStatement>& child, IProcessContext* context, IParentContext* parentInfo)
    {
        return (child->evaluate(context, parentInfo) ?
                CStatement::ChildPredicate_Match | CStatement::ChildPredicate_Stop :
                CStatement::ChildPredicate_NoMatch | CStatement::ChildPredicate_Continue);
    }

protected:
    IStatement* m_defaultStatement = nullptr;
private:
    Children::iterator m_defaultPosition;
};

class CIfStatement : public CEvaluableStatement
{
#if 0
    <xsdl:if test="xpath">
        <!-- stataements -->
    </xsdl:if>
#endif
public:
    CIfStatement();
    ~CIfStatement();
};

class CChooseStatement : public TConditionalSelectorStatement<chooseTag, whenTag, otherwiseTag>
{
#if 0
    <xsdl:choose>
        <xsdl:when test="xpath"><!-- statements --></xsdl:when>
        <!-- repeated when statements -->
        <xsdl:otherwise><!-- statements --></xsdl:otherwise>
    </xsdl:choose>
#endif
public:
    CChooseStatement();
    ~CChooseStatement();

    class CWhenStatement : public CEvaluableStatement
    {
    public:
        CWhenStatement();
        ~CWhenStatement();
    };

    class COtherwiseStatement : public CConditionalDefaultStatement
    {
    public:
        COtherwiseStatement();
        ~COtherwiseStatement();
    };
};

class CSwitchStatememnt : public TConditionalSelectorStatement<switchTag, caseTag, defaultTag>
{
#if 0
    <xsdl:switch test="xpath">
        <xsdl:case value="xpath"><!-- statements --></xsdl:case>
        <!-- repeated case statements -->
        <xsdl:default><!-- statements --></xsdl:default>
    </xsdl:switch>
#endif
public:
    CSwitchStatememnt();
    ~CSwitchStatememnt();

    class CCaseStatement : public CComparableStatement
    {
    public:
        CCaseStatement();
        ~CCaseStatement();
    };

    class CDefaultStatement : public CConditionalDefaultStatement
    {
    public:
        CDefaultStatement();
        ~CDefaultStatement();
    };

protected:
    bool initializeSelf(ILoadContext& context) override;
    bool processSelf(IProcessContext& context, IParentContext* parentInfo, const char* readXPath, const char* writeXPath, Owned<IParentContext>& selfInfo) const override;
private:
    StringBuffer m_testAttribute;
    Owned<ICompiledXpath> m_compiledXPath;
};

#endif // _EsConditionalAtatements_IPP__
