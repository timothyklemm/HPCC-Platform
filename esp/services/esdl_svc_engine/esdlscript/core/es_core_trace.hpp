#ifndef _EsdlScriptCoreTrace_HPP_
#define _EsdlScriptCoreTrace_HPP_

#include "esp.hpp"
#include "es_core_utility.hpp"
#include <map>

namespace EsdlScript
{
namespace Trace
{

enum Category
{
    Category_Undefined = -1,
    Category_Disaster,
    Category_AuditError,
    Category_InternalError,
    Category_OperatorError,
    Category_UserError,
    Category_InternalWarning,
    Category_OperatorWarning,
    Category_UserWarning,
    Category_DeveloperProgress,
    Category_OperatorProgress,
    Category_UserProgress,
    Category_DeveloperInfo,
    Category_OperatorInfo,
    Category_UserInfo,
    Category_Stats,
};

using CategoryLevels = std::map<Category, LogLevel>;

using Audience = uint8_t;
static const Audience Audience_Undefined = 0;
static const Audience Audience_Developer = 1;
static const Audience Audience_Operator = 2;
static const Audience Audience_User = 4;
static const Audience Audience_Audit = 8;
static const Audience Audience_All = Audience_Developer | Audience_Operator | Audience_User | Audience_Audit;

enum CategoryGroup
{
    CategoryGroup_Undefined = -1,
    CategoryGroup_Error,
    CategoryGroup_Warning,
    CategoryGroup_Progress,
    CategoryGroup_Info,
};

extern LogLevel getDefaultLogLevel(Category category);
extern LogLevel getLogLevel(Category category, bool fallback = true);
extern void setLogLevel(Category category, LogLevel level);
extern void resetLogLevel(Category category);
extern void setLogLevel(Audience audience, LogLevel level);
extern void resetLogLevel(Audience audience);
extern void setLogLevel(CategoryGroup group, LogLevel level);
extern void resetLogLevel(CategoryGroup group);

void trace(Category category, int code, const char* format, va_list& args);

#define VARIADIC_TRACE(c) \
    va_list args; \
    va_start(args, format); \
    trace((c), code, format, args); \
    va_end(args)

inline void trace(Category category, int code, const char* format, ...) { VARIADIC_TRACE(category); }
inline void disaster(int code, const char* format, ...) { VARIADIC_TRACE(Category_Disaster); }
inline void auditError(int code, const char* format, ...) { VARIADIC_TRACE(Category_AuditError); }
inline void internalError(int code, const char* format, ...) { VARIADIC_TRACE(Category_InternalError); }
inline void operatorError(int code, const char* format, ...) { VARIADIC_TRACE(Category_OperatorError); }
inline void userError(int code, const char* format, ...) { VARIADIC_TRACE(Category_UserError); }
inline void internalWarning(int code, const char* format, ...) { VARIADIC_TRACE(Category_InternalWarning); }
inline void operatorWarning(int code, const char* format, ...) { VARIADIC_TRACE(Category_OperatorWarning); }
inline void userWarning(int code, const char* format, ...) { VARIADIC_TRACE(Category_UserWarning); }
inline void developerProgress(int code, const char* format, ...) { VARIADIC_TRACE(Category_DeveloperProgress); }
inline void operatorProgress(int code, const char* format, ...) { VARIADIC_TRACE(Category_OperatorProgress); }
inline void userProgress(int code, const char* format, ...) { VARIADIC_TRACE(Category_UserProgress); }
inline void developerInfo(int code, const char* format, ...) { VARIADIC_TRACE(Category_DeveloperInfo); }
inline void operatorInfo(int code, const char* format, ...) { VARIADIC_TRACE(Category_OperatorInfo); }
inline void userInfo(int code, const char* format, ...) { VARIADIC_TRACE(Category_UserInfo); }
inline void stats(int code, const char* format, ...) { VARIADIC_TRACE(Category_Stats); }

#undef VARIADIC_TRACE

} // namespace Trace

// Provides persistence of the trace settings.
interface ITraceState : extends IInterface, extends IPersistent
{
};

// On construction, saves the current trace settings. On destruction, restores
// the saved settings.
struct StTraceState
{
    StTraceState();
    ~StTraceState();

private:
    Trace::CategoryLevels m_savedState;
};

// On construction, sets the provided contextual strings for use in trace
// statements. On destruction, restores the prior values.
struct StTraceContext
{
    StTraceContext(const char* component, const char* operation);
    StTraceContext(const char* operation);
    ~StTraceContext();
    void updateOp(const char* operation);

private:
    Linked<String> m_savedComponent;
    Linked<String> m_savedOperation;
};

interface IHistoricalEvent : extends IInterface
{
    enum Type
    {
        Undefined = -1,
        Progress,
        Info,
        Warning,
        Error,
        Fatal,
    };

    virtual Type getType() const = 0;
    virtual int getCode() const = 0;
    virtual const char* getMessage() const = 0;

    inline bool isFailure() const { Type t = getType(); return (Fatal == t || Error || t); }
    inline bool isWarning() const { return getType() == Warning; }
    inline bool isSuccess() const { Type t = getType(); return (Progress == t || Info == t); }
};

interface IHistoricalEventIterator : extends IIteratorOf<const IHistoricalEvent> {};

interface IHistory : extends IInterface, extends IPersistent
{
    using Iterator = IHistoricalEventIterator;

    enum State
    {
        Success,
        Warned,
        Failed,
    };

    virtual bool getWarningAsFailure() const = 0;
    virtual void setWarningAsFailure(bool state) = 0;
    virtual State getState() const = 0;
    inline bool hasFailed() const { return getState() == Failed; }
    inline bool hasWarned() const { return getState() == Warned; }
    inline bool hasSucceeded() const { return getState() == Success; }

    virtual void record(Trace::Category, int code, const char* message) = 0;
    virtual void reset(bool state = true, bool events = true) = 0;
    virtual Iterator* getEvents() const = 0;
};
DECLARE_IOWNER(History);

// On construction, set or clear the active history instance of the current
// thread. On destruction, restore the original state.
struct StActiveHistory
{
    StActiveHistory(IHistory* history);
    ~StActiveHistory();

private:
    Linked<IHistory> m_savedActiveHistory;
};

// Returns the currently active history instance. Query returns the instance
// without a reference; get adds a reference.
extern IHistory* queryCurrentEvents();
extern IHistory* getCurrentEvents();

} // namespace EsdlScript

#endif // _EsdlScriptCoreTrace_HPP_
