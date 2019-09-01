#ifndef _EsdlScriptCoreTrace_IPP_
#define _EsdlScriptCoreTrace_IPP_

#include "es_core_trace.hpp"
#include "es_core_utility.ipp"
#include <list>

namespace EsdlScript
{

class CTraceState : implements ITraceState, extends CInterface, extends CPersistent
{
public:
    // IInterface
    IMPLEMENT_IINTERFACE;
    // IPersistent
    IMPLEMENT_IPERSISTENT("TraceState");
};

class CHistory : implements IHistory, extends CInterface, extends CPersistent
{
protected:
    using Type = IHistoricalEvent::Type;
    class CEvent : implements IHistoricalEvent, extends CInterface
    {
    public:
        CEvent(Type type, int code, const char* message) : m_type(type), m_code(code), m_message(message) {}
        ~CEvent() {};
        // IInterface
        IMPLEMENT_IINTERFACE;
        // IHistoricalEvent
        Type getType() const override { return m_type; }
        int getCode() const override { return m_code; }
        const char* getMessage() const override { return m_message; }
    protected:
        Type m_type;
        int m_code;
        StringBuffer m_message;
    };
    using Events = std::list<Owned<CEvent> >;
    class CEventIterator : implements CInterfaceOf<IHistoricalEventIterator>
    {
    public:
        CEventIterator(const CHistory& history);
        ~CEventIterator();
        bool first() override;
        bool next() override;
        bool isValid() override;
        IHistoricalEvent& query() override;
    protected:
        Linked<const CHistory> m_history;
        Events::const_iterator m_eventIt;
    };

public:
    CHistory(const char* context);
    ~CHistory();
    // IInterface
    IMPLEMENT_IINTERFACE;
    // IPersistent
    IMPLEMENT_IPERSISTENT_COLLECTION("History", "events");
    // IHistory
    bool getWarningAsFailure() const override { return m_warningAsFailure; }
    void setWarningAsFailure(bool state) override { m_warningAsFailure = state; }
    State getState() const;
    void record(Trace::Category category, int code, const char* message) override;
    void reset(bool state = true, bool events = true) override;
    Iterator* getEvents() const override;

    void record(Type type, int code, const char* message);

private:
    StringBuffer m_context;
    Events m_events;
    bool m_warningAsFailure = false;
    Type m_cumulativeType = Type::Undefined;
};

} // namespace EsdlScript

#endif // _EsdlScriptCoreTrace_IPP_
