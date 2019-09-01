#ifndef _EsdlScript_HPP_
#define _EsdlScript_HPP_

#include "esp.hpp"
#include "es_core_utility.hpp"
#include "es_core_statement.hpp"
#include "es_core_trace.hpp"
#include <list>

namespace EsdlScript
{

enum Phase
{
    Phase_Unknown = -1,
    Phase_Preflight,
    Phase_Request,
    Phase_RawResponse,
    Phase_FinalResponse,
    Phase_LogManager,
    Phase_LogAgent,
};

enum ProcessResult
{
    ProcessResult_Unknown = -1,
    ProcessResult_Success,             // all statements completed without error
    ProcessResult_Aborted,             // statement processing terminated without error
    ProcessResult_Failed,              // statement processing terminated with error
    ProcessResult_AbnormalTermination, // statement processing exception
};

struct ScriptFragment
{
    StringBuffer content;
    const char* service = nullptr;
    const char* method = nullptr;
};

class Script : extends std::list<ScriptFragment>
{
public:
    using Fragment = ScriptFragment;
    using Base = std::list<Fragment>;
    using Base::Base;

    void add(const IPTree* tree, const char* xpath, const char* service, const char* context = nullptr);
};

interface ILoadContext : extends IInterface, extends IXMLParserOwner, extends IHistoryOwner, extends IStatementFactoryOwner
{
    virtual void addServiceConstraint(const char* service) = 0;
    virtual void removeServiceConstraint(const char* service) = 0;
    virtual void clearServiceConstraints() = 0;
    virtual bool isServiceConstraint(const char* service) const = 0;

    virtual void addPhaseExclusion(Phase phase) = 0;
    virtual void removePhaseExclusion(Phase phase) = 0;
    virtual void clearPhaseExclusion() = 0;
    virtual bool isPhaseExclusion(Phase phase) const = 0;

    virtual void setService(const char* name) = 0;
    virtual const char* getService() const = 0;
    virtual void setMethod(const char* name) = 0;
    virtual const char* getMethod() const = 0;
};
DECLARE_IOWNER(LoadContext);

interface IRequestProcessor : extends IInterface, extends IPersistent, extends IXMLParserOwner, extends IHistoryOwner
{
    virtual bool prepareRequest(const IPTree* espRequest, StringBuffer& backEndRequest) = 0;
    virtual bool processResponse(const char* backEndResponse, StringBuffer& espResponse) = 0;
    virtual bool prepareLogContent(IPTree*& logContent) = 0;
};
DECLARE_IOWNER(RequestProcessor);

interface ILogProcessor : extends IInterface, extends IXMLParserOwner, extends IHistoryOwner
{
    virtual bool prepareUpdateContent(StringBuffer& content) = 0;
};
DECLARE_IOWNER(LogProcessor);

// An encapsulation of dynamic ESDL script logic.
//
// 1. The script library should be loaded with the binding. It is the caller's
//    responsibility to supply the library content.
//    * If a binding tree is given, the relevant pieces will be loaded and all
//      else will be ignored.
//    * If script fragments are given, only the relevant pieces are accepted.
//    * Multiple calls to load are permitted, but duplicate definitions are not.
// 2. A new request processor should be created for each service request.
// 3. A new log processor should be created for each log update request.
interface IEnvironment : extends IInterface, extends IPersistent, extends ILoadContextOwner
{
    virtual bool load(const IPTree* binding) = 0;
    virtual bool load(const Script& script) = 0;
    virtual void reset() = 0;

    virtual IRequestProcessor* createRequestProcessor(IEspContext& espContext, const char* service, const char* method) const = 0;
    virtual ILogProcessor* createLogProcessor(const char* logXML) const = 0;
};

} // namespace EsdlScript

EsdlScript::IEnvironment* createEsdlScriptEnvironment(const char* service);

#endif // _EsdlScript_HPP_
