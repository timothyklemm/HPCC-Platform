#include "es_update_statements.ipp"

namespace EsdlScript
{

const char* setValueTag = "xsdl:SetValue";
const char* appendValueTag = "xsdl:appendValue";

const char* targetAttribute = "target";
const char* valueAttribute = "value";

CValueStatement::CValueStatement()
{
}

CValueStatement::~CValueStatement()
{
}

bool CValueStatement::initializeSelf(ILoadContext& context)
{
    auto target = context.currentAttribute(targetAttribute);
    auto value = context.currentAttribute(valueAttribute);
    auto result = true;

    m_targetXPath.append(context.currentAttribute(targetAttribute));
    if (m_targetXPath.isEmpty())
    {
        context.queryOutcomes()->recordError(-1, "Statement '%s' cannot have an empty %s attribute", queryTag(), targetAttribute);
    }
    else if (nullptr == value)
    {
        // omitted value --> nested statements are accepted
        setAcceptsChildren(true);
    }
    else if ('\0' == *value)
    {
        // blank value --> value given and no nested statements will be accepted
        setAcceptsChldren(false);
    }
    else
    {
        // value given and no nested statements will be accepted
        setAcceptsChildren(false);
        m_compiledValueXPath.setown(compileXpath(value));
    }

    return result;
}

void CValueStatement::validateSelf(ILoadContext& context)
{
    if (acceptsChildren() && m_children.empty())
    {
        context.queryOutcomes()->recordError(-1, "Statement '%s' requires either a value or nested statements.", queryTag());
    }
}

bool CValueStatement::processSelf(IProcessContext& context, IParentContext* parentInfo, const char* readXPath, const char* writeXPath, Owned<IParentContext>& selfInfo)
{
    auto writeCursor = context.queryWriteCursor();
    auto root = writeCursor->resolveRoot();
    auto node = root->ensurePTree(m_targetXPath);

    if (!acceptsChildren())
    {
        StringBuffer value;

        if (m_compiledValueXPath.get() != nullptr)
        {
            auto readCursor = context.queryReadCursor();

            readCursor->evaluate(m_compiledValueXPath, value);
        }
        applyValue(context, node, value);
    }
}

CSetValueStatement::CSetValueStatement()
{
}

CSetValueStatement::~CSetValueStatement()
{
}

void CSetValueStatement::applyValue(IProcessContext& context, IPTree* node, const char* xpath, const char* value) const
{
    node->setProp(xpath, value);
}

CAppendValueStatement::CAppendValueStatement()
{
}

CAppendValueStatement::~CAppendValueStatement()
{
}

void CAppendValueStatement::applyValue(IProcessContext& context, IPTree* node, const char* xpath, const char* value) const
{
    node->appendProp(xpath, value);
}

} // namespace EsdlScript
