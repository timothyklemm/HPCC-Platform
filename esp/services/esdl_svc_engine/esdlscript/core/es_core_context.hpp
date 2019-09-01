#ifndef _EsdlScriptCoreContext_HPP_
#define _EsdlScriptCoreContext_HPP_

#include "esp.hpp"
#include "es_core_logging.hpp"
#include "es_core_trace.hpp"
#include "es_core_utility.hpp"

namespace EsdlScript
{

interface ICoreContext : extends IInterface, extends IXMLParserOwner
{
};

interface ILoadContext : extends ICoreContext
{
};

interface IProcessContext : extends ICoreContext, extends ILogAgentStateOwner
{
};

interface IRestoreContext : extends ICoreContext, extends ILogAgentStateOwner
{
};

} // namespace EsdlScript

#endif // _EsdlScriptCoreContext_HPP_
