#include "es_core_logging.ipp"
#include "es_core_trace.hpp"

namespace EsdlScript
{

CLogAgentFilter::CLogAgentFilter(const Variants& variants, LogAgentFilterMode mode, LogAgentFilterType type, const char* pattern)
    : m_mode(mode)
    , m_type(type)
    , m_pattern(pattern)
{
    for (Variant v : variants)
    {
        switch (type)
        {
        case LAFT_Unfiltered:
            doMatch(true, v, mode);
            break;
        case LAFT_Group:
            doMatch(isMatch(v->getGroup(), pattern), v, mode);
            break;
        case LAFT_Type:
            doMatch(isMatch(v->getType(), pattern), v, mode);
            break;
        case LAFT_Name:
            doMatch(isMatch(v->getName(), pattern), v, mode);
            break;
        default:
            break;
        }
    }
}

CLogAgentFilter::~CLogAgentFilter()
{
}

ILogAgentFilter* CLogAgentFilter::refine(LogAgentFilterMode mode, LogAgentFilterType type, const char* pattern)
{
    if (!m_refinements.empty() && m_refinements.front()->getMode() != mode)
    {
        // inclusive and exclusive siblings are contradictory
        return nullptr;
    }

    Owned<ILogAgentFilter> filter(new CLogAgentFilter(m_variants, mode, type, pattern));

    return filter.getClear();
}

void CLogAgentFilter::reset()
{
    m_refinements.clear();
}

bool CLogAgentFilter::includes(Variant agent, bool recurse) const
{
    bool result = false;
    if (!recurse || m_refinements.empty())
    {
        result = m_variants.find(agent) != m_variants.end();
    }
    else
    {
        for (const Owned<ILogAgentFilter>& f : m_refinements)
        {
            if (f->includes(agent, recurse))
            {
                result = true;
                break;
            }
        }
    }
    return result;
}

bool CLogAgentFilter::isMatch(const char* value, const char* pattern) const
{
    if (isEmptyString(pattern))
        return isEmptyString(value);
    if (streq(pattern, "*"))
        return true;
    if (isEmptyString(value))
        return false;
    return strieq(value, pattern);
}

bool CLogAgentFilter::doMatch(bool matched, Variant variant, LogAgentFilterMode mode)
{
    if ((LAFM_Inclusive == mode) == matched)
        m_variants.insert(variant);
    else
        matched = false;
    return matched;
}

CLogAgentState::CLogAgentState(const ILoggingManager* manager)
    : CLogAgentFilter(transform(manager), LAFM_Inclusive, LAFT_Unfiltered, nullptr)
{

}

CLogAgentState::~CLogAgentState()
{
}

StringBuffer& CLogAgentState::persist(StringBuffer& xml) const
{
    xml.appendf("<LogAgentState variants=\"%zu\">", m_variants.size());
    for (Variant v : m_variants)
    {
        xml << "<Variant";
        if (v->getName() != nullptr)
            xml << " name=\"" << v->getName() << "\"";
        if (v->getType() != nullptr)
            xml << " type=\"" << v->getType() << "\"";
        if (v->getGroup() != nullptr)
            xml << " group=\"" << v->getGroup() << "\"";
        xml << " enabled=\"" << int(includes(v)) << "\"/>";
    }
    xml.append("</LogAgentState>");

    return xml;
}

void CLogAgentState::restoreInstance(IXMLParser& parser, const char* tag, size_t variantCnt)
{
    // Assuming that this->m_variants contains a complete set of variants,
    // restoration involves identifying the subset of variants that are
    // enabled.

    // This is safe for its intended purpose of using local variables to
    // perform set lookups. It is not intended for any other purpose.
    class CVariant : public CInterfaceOf<const ILogAgentVariant>
    {
    public:
        const char* name;
        const char* type;
        const char* group;

        const char* getName() const override { return name; }
        const char* getType() const override { return type; }
        const char* getGroup() const override { return group; }
    };

    Owned<CVariant> needle(new CVariant);
    Variants tmp;
    size_t variantIdx = 0;
    bool enabled = false;

    while (parser.next() && !parser.atEndTag(tag))
    {
        if (parser.atStartTag("Variant"))
        {
            restoring(parser.currentTag(), ++variantIdx, variantCnt);
            needle->name = parser.currentAttribute("name");
            if (isEmptyString(needle->name))
            {
                Trace::internalError(0, "missing Variant/@name");
                continue;
            }
            if (!parser.currentAttribute("enabled", enabled))
            {
                Trace::internalError(0, "missing Variant/@enabled");
                continue;
            }
            if (!enabled)
                continue;
            needle->type = parser.currentAttribute("type");
            needle->group = parser.currentAttribute("group");

            Variants::iterator it = m_variants.find(needle);
            if (it != m_variants.end())
                tmp.insert(*it); // keep the matching variant, not the needle
            else
                Trace::developerInfo(0, "variant {%s, '%s', '%s'} not configured", needle->name, (needle->type ? needle->type : ""), (needle->group ? needle->group : ""));
        }
        else if (parser.atStartTag())
        {
            parser.skip();
        }
    }

    reset();
    m_variants = tmp;
}

CLogAgentFilter::Variants CLogAgentState::transform(const ILoggingManager* manager)
{
    Owned<IEspLogAgentVariantIterator> variants(manager->getAgentVariants());
    Variants result;

    ForEach(*variants)
    {
        result.insert(&variants->query());
    }
    return result;
}

} // namespace EsdlScript
