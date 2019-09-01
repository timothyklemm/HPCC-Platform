#ifndef _EsdlScriptCoreEnvironment_IPP_
#define _EsdlScriptCoreEnvironment_IPP_

#include "esdlscript.hpp"

namespace EsdlScript
{

class CEnvironment : implements IEnvironment, extends CInterface
{
public:
    CEnvironment();
    IMPLEMENT_IINTERFACE;

    // IEnvironment
    bool load(const IPTree* binding) override;
    bool load(const Script& script) override;
    void reset() override;
    IRequestProcessor* createRequestProcessor(IEspContext& espContext, const char* service, const char* method) const override;
    ILogProcessor* createLogProcessor(const char* logXML) const override;
}

} // namespace EsdlScript

#endif // _EsdlScriptCoreEnvironment_IPP_
