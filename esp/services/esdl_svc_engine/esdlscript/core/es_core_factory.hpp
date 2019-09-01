#ifndef _EsdlScriptCoreFactory_HPP_
#define _EsdlScriptCoreFactory_HPP_

#include "es_core_trace.hpp"
#include "esp.hpp"
#include <functional>
#include <map>

namespace EsdlScript
{

template <typename TInterface>
interface IFactory : extends IInterface
{
    using Creator = std::function<TInterface*()>;

    virtual bool registerTag(const char* tag, Creator creator) = 0;
    virtual bool isRegisteredTag(const char* tag) const = 0;
    virtual TInterface* create(const char* tag) const = 0;
    virtual bool eraseTag(const char* tag) = 0;
};

} // namespace EsdlScript

#endif // _EsdlScriptCoreFactory_HPP_
