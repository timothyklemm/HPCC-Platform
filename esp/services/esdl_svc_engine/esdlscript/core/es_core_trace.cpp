#include "es_core_trace.ipp"
#include "espcontext.hpp"
#include "jlog.hpp"
#include <map>
#include <set>

namespace EsdlScript
{

namespace Trace
{

namespace
{
using CategoryLevelMap = std::map<Category, LogLevel>;
const CategoryLevelMap defaultCategoryLevels({
    { Category_Disaster, LogMin },
    { Category_AuditError, LogMin },
    { Category_InternalError, LogNone },
    { Category_OperatorError, LogMin },
    { Category_UserError, LogNone },
    { Category_InternalWarning, LogNone },
    { Category_OperatorWarning, LogMin },
    { Category_UserWarning, LogNone },
    { Category_DeveloperProgress, LogNone },
    { Category_OperatorProgress, LogMin },
    { Category_UserProgress, LogNone },
    { Category_DeveloperInfo, LogNone },
    { Category_OperatorInfo, LogMin },
    { Category_UserInfo, LogNone },
    { Category_Stats, LogMin },
});
thread_local CategoryLevelMap currentCategoryLevels;

using AudienceCategoriesMap = std::map<Audience, std::set<Category> >;
const AudienceCategoriesMap defaultAudienceCategories({
    { Audience_Developer, { Category_Disaster, Category_InternalError, Category_InternalWarning, Category_DeveloperProgress, Category_DeveloperInfo } },
    { Audience_Operator, { Category_Disaster, Category_OperatorError, Category_OperatorWarning, Category_OperatorProgress, Category_OperatorInfo, Category_Stats } },
    { Audience_User, { Category_Disaster, Category_UserError, Category_UserWarning, Category_UserProgress, Category_UserInfo } },
    { Audience_Audit, { Category_Disaster, Category_AuditError } },
});

using GroupCategoriesMap = std::map<CategoryGroup, std::set<Category> >;
const GroupCategoriesMap defaultGroupCategories({
    { CategoryGroup_Error, { Category_Disaster, Category_AuditError, Category_InternalError, Category_OperatorError, Category_UserError } },
    { CategoryGroup_Warning, { Category_InternalWarning, Category_OperatorWarning, Category_UserWarning } },
    { CategoryGroup_Progress, { Category_DeveloperProgress, Category_OperatorProgress, Category_UserProgress } },
    { CategoryGroup_Info, { Category_DeveloperInfo, Category_OperatorInfo, Category_UserInfo } },
});

template <class TMap>
void setMappedLevel(const TMap& mapping, typename TMap::key_type key, LogLevel level)
{
    auto it = mapping.find(key);
    if (it != mapping.end())
    {
        for (Category c : it->second)
            setLogLevel(c, level);
    }
}

template <class TMap>
void resetMappedLevel(const TMap& mapping, typename TMap::key_type key)
{
    auto it = mapping.find(key);
    if (it != mapping.end())
    {
        for (Category c : it->second)
            resetLogLevel(c);
    }
}

thread_local Owned<String> currentComponent;
thread_local Owned<String> currentOperation;

void update(Owned<String>& container, const char* value)
{
    if (nullptr == value)
    {
        container.clear();
    }
    else if (nullptr == container.get() || container->compareTo(value) != 0)
    {
        container.setown(new String(value));
    }
}

thread_local Linked<IHistory> currentEvents;

} // empty namespace


LogLevel getDefaultLogLevel(Category category)
{
    CategoryLevelMap::const_iterator it = defaultCategoryLevels.find(category);
    if (it != defaultCategoryLevels.end())
        return it->second;
    return LogNone;
}

LogLevel getLogLevel(Category category, bool fallback)
{
    CategoryLevelMap::const_iterator it = currentCategoryLevels.find(category);
    if (it != currentCategoryLevels.end())
        return it->second;
    if (fallback)
        return getDefaultLogLevel(category);
    return LogNone;
}

void setLogLevel(Category category, LogLevel level)
{
    CategoryLevelMap::const_iterator it = defaultCategoryLevels.find(category);
    if (it != defaultCategoryLevels.end())
    {
        if (level == it->second)
            currentCategoryLevels.erase(category);
        else
            currentCategoryLevels[category] = level;
    }
}

void resetLogLevel(Category category)
{
    currentCategoryLevels.erase(category);
}

void setLogLevel(Audience audience, LogLevel level)
{
    setMappedLevel(defaultAudienceCategories, audience & Audience_Developer, level);
    setMappedLevel(defaultAudienceCategories, audience & Audience_Operator, level);
    setMappedLevel(defaultAudienceCategories, audience & Audience_User, level);
    setMappedLevel(defaultAudienceCategories, audience & Audience_Audit, level);
}

void resetLogLevel(Audience audience)
{
    resetMappedLevel(defaultAudienceCategories, audience & Audience_Developer);
    resetMappedLevel(defaultAudienceCategories, audience & Audience_Operator);
    resetMappedLevel(defaultAudienceCategories, audience & Audience_User);
    resetMappedLevel(defaultAudienceCategories, audience & Audience_Audit);
}

void setLogLevel(CategoryGroup group, LogLevel level)
{
    setMappedLevel(defaultGroupCategories, group, level);
}

void resetLogLevel(CategoryGroup group)
{
    resetMappedLevel(defaultGroupCategories, group);
}

void trace(Category category, int code, const char* fmt, va_list& args)
{
    // update the format string based on current component, operation values
    StringBuffer format;
    if (currentComponent && currentOperation)
        format << '[' << currentComponent->str() << '|' << currentOperation->str() << "] " << fmt;
    else if (currentComponent)
        format << '[' << currentComponent->str() << "] " << fmt;
    else if (currentOperation)
        format << '[' << currentOperation->str() << "] " << fmt;
    else
        format << fmt;

    StringBuffer msg;
    msg.valist_appendf(format, args);

    LogLevel effectiveLevel = getLogLevel(category);
    if (LogNone < effectiveLevel && effectiveLevel <= getEspLogLevel())
    {
        switch (category)
        {
        case Category_Disaster:
            DISLOG("%d: %s", code, msg.str());
            break;
        case Category_AuditError:
            AERRLOG("%d: %s", code, msg.str());
            break;
        case Category_InternalError:
            IERRLOG("%d: %s", code, msg.str());
            break;
        case Category_OperatorError:
            OERRLOG("%d: %s", code, msg.str());
            break;
        case Category_UserError:
            UERRLOG("%d: %s", code, msg.str());
            break;
        case Category_InternalWarning:
            IWARNLOG("%d: %s", code, msg.str());
            break;
        case Category_OperatorWarning:
            OWARNLOG("%d: %s", code, msg.str());
            break;
        case Category_UserWarning:
            UWARNLOG("%d: %s", code, msg.str());
            break;
        case Category_DeveloperProgress:
            LOG(MCdebugProgress, "%d: %s", code, msg.str());
            break;
        case Category_OperatorProgress:
            LOG(MCoperatorProgress, "%d: %s", code, msg.str());
            break;
        case Category_UserProgress:
            PROGLOG("%d: %s", code, msg.str());
            break;
        case Category_DeveloperInfo:
            DBGLOG("%d: %s", code, msg.str());
            break;
        case Category_OperatorInfo:
            LOG(MCoperatorInfo, "%d: %s", code, msg.str());
            break;
        case Category_UserInfo:
            LOG(MCuserInfo, "%d: %s", code, msg.str());
            break;
        case Category_Stats:
            LOG(MCstats, "%d: %s", code, msg.str());
            break;
        }
    }

    if (currentEvents)
        currentEvents->record(category, code, msg);
}

} // namespace Trace

namespace
{
StringBuffer& persistCategory(Trace::Category category, StringBuffer& xml)
{
    xml << "<Category"
        << " id=\"" << int(category) << "\""
        << " level=\"" << int(Trace::getLogLevel(category)) << "\""
        << "/>";
    return xml;
}
} // empty namespace

StringBuffer& CTraceState::persist(StringBuffer& xml) const
{
    xml << "<TraceState>";
    for (auto& entry : Trace::defaultCategoryLevels)
        persistCategory(entry.first, xml);
    xml << "</TraceState>";
    return xml;
}

void CTraceState::restoreInstance(IXMLParser& parser, const char* tag, size_t count)
{
    while (parser.next() && !parser.atEndTag(tag))
    {
        if (parser.atStartTag("Category"))
        {
            Trace::Category category;
            LogLevel level;

            if (!parser.currentAttribute("id", category))
            {
                break;
            }
            if (!parser.currentAttribute("level", level))
            {
                break;
            }
            Trace::setLogLevel(category, level);
        }
        else if (parser.atStartTag())
        {
            parser.skip();
        }
    }
}

StTraceState::StTraceState()
{
    m_savedState = Trace::currentCategoryLevels;
}

StTraceState::~StTraceState()
{
    Trace::currentCategoryLevels = m_savedState;
}

StTraceContext::StTraceContext(const char* component, const char* operation)
    : m_savedComponent(Trace::currentComponent)
    , m_savedOperation(Trace::currentOperation)
{
    Trace::update(Trace::currentComponent, component);
    Trace::update(Trace::currentOperation, operation);
}

StTraceContext::StTraceContext(const char* operation)
    : m_savedComponent(Trace::currentComponent)
    , m_savedOperation(Trace::currentOperation)
{
    Trace::update(Trace::currentOperation, operation);
}

StTraceContext::~StTraceContext()
{
    if (m_savedComponent.get() != nullptr)
        Trace::update(Trace::currentComponent, m_savedComponent->str());
    if (m_savedOperation.get() != nullptr)
        Trace::update(Trace::currentOperation, m_savedOperation->str());
}

CHistory::CEventIterator::CEventIterator(const CHistory& history)
    : m_history(&history)
    , m_eventIt(history.m_events.end())
{
}

CHistory::CEventIterator::~CEventIterator()
{
}

bool CHistory::CEventIterator::first()
{
    m_eventIt = m_history->m_events.begin();
    return isValid();
}

bool CHistory::CEventIterator::next()
{
    ++m_eventIt;
    return isValid();
}

bool CHistory::CEventIterator::isValid()
{
    return (m_eventIt != m_history->m_events.end());
}

IHistoricalEvent& CHistory::CEventIterator::query()
{
    if (!isValid())
        throw MakeStringException(0, "CEventIterator::query called in invalid state");
    IHistoricalEvent* entry = m_eventIt->get();
    if (nullptr == entry)
        throw MakeStringException(0, "CEventIterator::query encountered a NULL event");
    return *entry;
}

CHistory::CHistory(const char* context)
    : m_context(context)
{
}

CHistory::~CHistory()
{
}

StringBuffer& CHistory::persist(StringBuffer& xml) const
{
    xml.appendf("<History context=\"%s\" events=\"%zu\"", m_context.str(), m_events.size());
    if (m_events.size() == 0)
        xml << "/>";
    else
    {
        xml << ">";
        for (const Owned<CEvent>& e : m_events)
        {
            xml << "<Event"
                << " type=\"" << e->getType() << "\""
                << " code=\"" << e->getCode() << "\""
                << " message=\"" << e->getMessage() << "\""
                << "/>";
        }
        xml << "</History>";
    }
    return xml;
}

void CHistory::restoreInstance(IXMLParser& parser, const char* tag, size_t eventCount)
{
    size_t eventIdx = 0;
    m_context.set(parser.currentAttribute("context"));
    m_events.clear();
    m_cumulativeType = IHistoricalEvent::Undefined;
    while (parser.next() && !parser.atEndTag(tag))
    {
        if (parser.atStartTag("Event"))
        {
            Type type = Type::Undefined;
            int code = 0;
            const char* message = nullptr;
            bool good = true;

            restoring(parser.currentTag(), ++eventIdx, eventCount);
            if (!parser.currentAttribute("type", type))
            {
                Trace::internalWarning(0, "missing Event/@type");
                good = false;
            }
            if (!parser.currentAttribute("code", code))
            {
                Trace::internalWarning(0, "missing Event/@code");
                good = false;
            }
            if ((message = parser.currentAttribute("message")) == nullptr)
            {
                Trace::internalWarning(0, "missing Event/@message");
                good = false;
            }
            if (good)
            {
                record(type, code, message);
            }
        }
        else if (parser.atStartTag())
        {
            parser.skip();
        }
    }
}

IHistory::State CHistory::getState() const
{
    switch (m_cumulativeType)
    {
    case Type::Fatal:
    case Type::Error:
        return Failed;
    case Type::Warning:
        return (m_warningAsFailure ? Failed : Warned);
    case Type::Progress:
    case Type::Info:
    default:
        return Success;
    }
}

void CHistory::record(Trace::Category category, int code, const char* message)
{
    using CategoryTypeMap = std::map<Trace::Category, IHistoricalEvent::Type>;
    static const CategoryTypeMap defaultCategoryTypes({
        { Trace::Category_Disaster, IHistoricalEvent::Fatal },
        { Trace::Category_AuditError, IHistoricalEvent::Error },
        { Trace::Category_InternalError, IHistoricalEvent::Error },
        { Trace::Category_OperatorError, IHistoricalEvent::Error },
        { Trace::Category_UserError, IHistoricalEvent::Error },
        { Trace::Category_InternalWarning, IHistoricalEvent::Warning },
        { Trace::Category_OperatorWarning, IHistoricalEvent::Warning },
        { Trace::Category_UserWarning, IHistoricalEvent::Warning },
        { Trace::Category_DeveloperProgress, IHistoricalEvent::Progress },
        { Trace::Category_OperatorProgress, IHistoricalEvent::Progress },
        { Trace::Category_UserProgress, IHistoricalEvent::Progress },
        { Trace::Category_DeveloperInfo, IHistoricalEvent::Info },
        { Trace::Category_OperatorInfo, IHistoricalEvent::Info },
        { Trace::Category_UserInfo, IHistoricalEvent::Info },
        { Trace::Category_Stats, IHistoricalEvent::Info },
    });

    CategoryTypeMap::const_iterator it = defaultCategoryTypes.find(category);
    if (defaultCategoryTypes.end() == it)
    {
        // Category did not map to a type. Record the message as an Info event.
        // The failure to record an event as requested is noteworthy, but should
        // not by itself prevent the current operation from completing.
        VStringBuffer wrapper("unable to record event (%d:%s) using category %d", code, message, int(category));
        record(Type::Info, code, wrapper);
    }
    else
    {
        record(it->second, code, message);
    }
}

void CHistory::reset(bool state, bool events)
{
    if (state)
        m_cumulativeType = Type::Undefined;
    if (events)
        m_events.clear();
}

IHistory::Iterator* CHistory::getEvents() const
{
    return new CEventIterator(*this);
}

void CHistory::record(Type type, int code, const char* message)
{
    switch (type)
    {
    case Type::Undefined:
        // not a recordable type
        return;
    case Type::Progress:
        if (Type::Undefined == m_cumulativeType)
            m_cumulativeType = Type::Info;
        break;
    default:
        if (m_cumulativeType < type)
            m_cumulativeType = type;
        break;
    }
    m_events.push_back(new CEvent(type, code, message));
}

StActiveHistory::StActiveHistory(IHistory* history)
    : m_savedActiveHistory(Trace::currentEvents)
{
    Trace::currentEvents.set(history);
}

StActiveHistory::~StActiveHistory()
{
    Trace::currentEvents.set(m_savedActiveHistory);
}

IHistory* queryCurrentEvents()
{
    return Trace::currentEvents;
}

IHistory* getCurrentEvents()
{
    return Trace::currentEvents.getLink();
}

} // namespace EsdlScript
