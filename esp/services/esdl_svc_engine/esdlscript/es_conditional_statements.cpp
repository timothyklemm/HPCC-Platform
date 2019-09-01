
#include "es_conditional_statements.ipp"

namespace EsdlScript
{
const char* chooseTag = "xsdl:choose";
const char*     whenTag = "xsdl:when";
const char*     otherwiseTag = "xsdl:otherwise";
const char* switchTag = "xsdl:switch";
const char*     caseTag = "xsdl:case";
const char*     defaultTag = "xsdl:default";
const char* ifTag = "xsdl:if";

const char* testAttribute = "test";
const char* valueAttribute = "value";

class CComparableContext : public CInterfaceOf<IParentContext>
{
public:
    CComparableContext(const char* value)
        : m_value(value)
    {
    }

    StringBuffer m_value;
};

CEvaluableStatement::CEvaluableStatement(const char* testAttribute = testAttribute)
    : m_testAttribute(testAttribute)
{
}

CEvaluableStatement::~CEvaluableStatement()
{
}

bool CEvaluableStatement::isEvaluable() const
{
    return true;
}

bool CEvaluableStatement::evaluate(IProcessContext* context, IParentContext* parentInfo) const
{
    auto cursor = context->queryReadCursor();

    return cursor->evaluate(m_compiledXPath);
}

bool CEvaluableStatement::initializeSelf(ILoadContext& context)
{
    auto xpath = context.currentAttribute(m_testAttribute);

    if (isEmptyString(xpath))
    {
        return false;
    }

    m_compiledXPath.setown(compileXpath(xpath));
    return true;
}

bool CComparableStatement::evaluate(IProcessContext* context, IParentContext* parentInfo) const
{
    auto comparableInfo = dynamic_cast<CComparableContext*>(parentInfo);;
    if (nullptr == comparableInfo)
    {
        return false;
    }

    auto cursor = context->queryReadCursor();
    StringBuffer value;

    cursor->evaluate(m_compiledXPath, value);
    return streq(value, comparableInfo->m_value);
}

CConditionalDefaultStatement::CConditionalDefaultStatement()
{
}

CConditionalDefaultStatement::~CConditionalDefaultStatement()
{
}

bool CConditionalDefaultStatement::isEvaluable() const
{
    return true;
}

bool CConditionalDefaultStatement::evaluate() const
{
    return true;
}

CIfStatement::CIfStatement()
{
}

CIfStatement::~CIfStatement()
{
}

CChooseStatement::CChooseStatement() {}

CChooseStatement::~CChooseStatement() {}

CChooseStatement::CWhenStatement::CWhenStatement()
{
    setParentAcceptor(acceptableParent);
}

CChooseStatement::CWhenStatement::~CWhenStatement()
{
}

CChooseStatement::COtherwiseStatement::COtherwiseStatement()
{
    setParentAcceptor(acceptableParent);
}

CChooseStatement::COtherwiseStatement::~COtherwiseStatement()
{
}

CSwitchStatememnt::CSwitchStatememnt()
{
}

CSwitchStatememnt::~CSwitchStatememnt()
{
}

CSwitchStatememnt::CCaseStatement::CCaseStatement()
    : CComparableStatement(valueAttribute)
{
    setParentAcceptor(acceptableParent);
}

CSwitchStatememnt::CCaseStatement::~CCaseStatement()
{
}

CSwitchStatememnt::CDefaultStatement::CDefaultStatement()
{
    setParentAcceptor(acceptableParent);
}

CSwitchStatememnt::CDefaultStatement::~CDefaultStatement()
{
}

bool CSwitchStatememnt::initializeSelf(ILoadContext& context)
{
    auto xpath = context.currentAttribute(testAttribute);

    if (isEmptyString(xpath))
    {
        return false;
    }

    m_compiledXPath.setown(compileXpath(xpath));
    return true;
}

bool CSwitchStatememnt::processSelf(IProcessContext& context, IParentContext* parentInfo, const char* readXPath, const char* writeXPath, Owned<IParentContext>& selfInfo) const
{
    StringBuffer value;
    auto cursor = context.queryReadCursor();

    selfInfo.setown(new CComparableContext(cursor->evaluate(m_compiledXPath, value)));
    return true;
}

};
