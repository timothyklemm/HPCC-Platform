/*##############################################################################

    HPCC SYSTEMS software Copyright (C) 2013 HPCC Systems®.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
############################################################################## */

#ifndef _EsdlScriptFactory_IPP_
#define _EsdlScriptFactory_IPP_

#include "es_core_factory.hpp"
#include "es_core_trace.hpp"
#include "jexcept.hpp"
#include "jstring.hpp"
#include <functional>
#include <map>
#include <string>

namespace EsdlScript
{

struct StringBufferLess
{
    StringBufferLess(bool noCase = false)
        : m_comparisonFn(noCase ? stricmp : strcmp)
    {
    }

    bool operator () (const StringBuffer& lhs, const StringBuffer& rhs) const
    {
        return m_comparisonFn(lhs, rhs) < 0;
    }

private:
    std::function<int(const StringBuffer&, const StringBuffer&)> m_comparisonFn;
};

template <typename TInterface>
class TFactory : extends CInterface
{
public:
    using Creator = std::function<TInterface*()>;
    enum Behavior
    {
        Ignore,
        Warn,
        Fail,
        Abort
    };

    TFactory(Behavior reregistration = Ignore, Behavior unregistered = Warn, Behavior deregistration = Ignore)
        : m_creators(StringBufferLess(false))
        , m_reregistrationBehavior(reregistration)
        , m_unregisteredBehavior(unregistered)
        , m_deregistrationBehavior(deregistration)
    {
    }

    ~TFactory()
    {
    }

    virtual bool registerTag(const char* tag, Creator creator)
    {
        assertTag(tag);
        if (nullptr == creator)
        {
            Trace::internalError(0, "invalid factory creator");
            throw MakeStringException(0, "invalid factory creator");
        }

        bool result = true;
        StringBuffer tmp(tag);
        WriteLockBlock block(m_creatorLock);
        auto it = m_creators.find(tmp);

        if (it != m_creators.end())
        {
            switch (m_reregistrationBehavior)
            {
            case Ignore:
                Trace::developerProgress(0, "factory tag '%s' re-registered", tag);
                it->second = creator;
                break;
            case Warn:
                Trace::internalWarning(0, "factory tag '%s' re-registered", tag);
                it->second = creator;
                break;
            case Fail:
                Trace::internalError(0, "factory tag '%s' already registered", tag);
                result = false;
                break;
            case Abort:
                Trace::internalError(0, "factory tag '%s' already registered", tag);
                throw MakeStringException(0, "factory tag '%s' already registered", tag);
            default:
                Trace::internalError(0, "unknown factory re-registration behavior %d for tag '%s'", m_reregistrationBehavior, tag);
                throw MakeStringException(0, "unknown factory re-registration behavior %d for tag '%s'", m_reregistrationBehavior, tag);
            }
        }
        else
        {
            Trace::developerProgress(0, "factory tag '%s' registered", tag);
            m_creators[tmp] = creator;
        }

        return result;
    }

    virtual bool isRegisteredTag(const char* tag) const
    {
        StringBuffer tmp(tag);
        ReadLockBlock block(m_creatorLock);
        auto it = m_creators.find(tmp);

        return it != m_creators.end();
    }

    virtual TInterface* create(const char* tag) const
    {
        assertTag(tag);

        Owned<TInterface> instance;
        bool found = false;

        { // limit the block scoppe
            StringBuffer tmp(tag);
            ReadLockBlock block(m_creatorLock);
            auto it = m_creators.find(tmp);

            if (it != m_creators.end())
            {
                found = true;
                instance.setown((it->second)());
            }
        }

        if (!found)
        {
            switch (m_unregisteredBehavior)
            {
            case Ignore:
                break;
            case Warn:
                Trace::internalWarning(0, "unregistered factory tag '%s'", tag);
                break;
            case Fail:
                Trace::internalError(0, "unregistered factory tag '%s'", tag);
                break;
            case Abort:
                Trace::internalError(0, "unregistered factory tag '%s'", tag);
                throw MakeStringException(0, "unregistered factory tag '%s'", tag);
            default:
                Trace::internalError(0, "unknown factory unregistered behavior %d for tag '%s'", m_unregisteredBehavior, tag);
                throw MakeStringException(0, "unknown factory unregistered behavior %d for tag '%s'", m_unregisteredBehavior, tag);
            }
        }
        else if (instance.get() == nullptr)
        {
            Trace::internalWarning(0, "nothing created for factory tag '%s'", tag);
        }
        return instance.getClear();
    }

    virtual bool eraseTag(const char* tag)
    {
        assertTag(tag);

        StringBuffer tmp(tag);
        WriteLockBlock block(m_creatorLock);
        auto it = m_creators.find(tmp);
        bool result = true;

        if (it != m_creators.end())
        {
            m_creators.erase(it);
        }
        else
        {
            switch (m_deregistrationBehavior)
            {
            case Ignore:
                break;
            case Warn:
                Trace::internalWarning(0, "unregistered factory tag '%s'", tag);
                break;
            case Fail:
                Trace::internalError(0, "unregistered factory tag '%s'", tag);
                result = false;
                break;
            case Abort:
                Trace::internalError(0, "unregistered factory tag '%s'", tag);
                throw MakeStringException(0, "unregistered factory tag '%s'", tag);
            default:
                Trace::internalError(0, "unknown factory deregistration behavior %d for tag '%s'", m_deregistrationBehavior, tag);
                throw MakeStringException(0, "unknown factory deregistration behavior %d for tag '%s'", m_deregistrationBehavior, tag);
            }
        }
        return result;
    }

protected:
    virtual void assertTag(const char* tag) const
    {
        if (isEmptyString(tag))
        {
            Trace::internalError(0, "invalid factory tag '%s'", tag);
            throw MakeStringException(0, "invalid factory tag '%s'", tag);
        }
    }

protected:
    using CreatorMap = std::map<StringBuffer, Creator, StringBufferLess>;
    CreatorMap m_creators;
    mutable ReadWriteLock m_creatorLock;
    Behavior m_reregistrationBehavior;
    Behavior m_unregisteredBehavior;
    Behavior m_deregistrationBehavior;
};

#define IMPLEMENT_IFACTORY(T) \
    bool registerTag(const char* tag, TFactory<T>::Creator creator) override { return TFactory<T>::registerTag(tag, creator); } \
    bool isRegisteredTag(const char* tag) const override { return TFactory<T>::isRegisteredTag(tag); } \
    T* create(const char* tag) const override { return TFactory<T>::create(tag); } \
    bool eraseTag(const char* tag) override { return TFactory<T>::eraseTag(tag); }

}

#endif // _EsdlScriptFactory_IPP_
