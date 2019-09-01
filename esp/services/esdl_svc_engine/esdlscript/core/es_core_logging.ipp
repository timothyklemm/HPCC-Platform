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

#ifndef _EsdlScriptCoreLogging_IPP_
#define _EsdlScriptCoreLogging_IPP_

#include "es_core_logging.hpp"
#include "es_core_utility.ipp"
#include "loggingmanager.h"
#include <list>
#include <set>

namespace EsdlScript
{

class CLogAgentFilter : implements ILogAgentFilter, extends CInterface
{
protected:
    using Filter = Owned<ILogAgentFilter>;
    using Filters = std::list<Filter>;
    using Variant = const ILogAgentVariant*;
#if 0
    using Variants = std::set<Linked<Variant>, IEspLogAgentVariantComparator>;
#else
    struct VariantComparator
    {
        bool operator () (const Variant& lhs, const Variant& rhs) const
        {
            return compare(lhs, rhs) < 0;
        }

    private:
        int compare(const Variant& lhs, const Variant& rhs) const
        {
            if (lhs == rhs)
                return 0;
            if (nullptr == lhs)
                return 1;
            if (nullptr == rhs)
                return -1;
            int relation = compare(lhs->getName(), rhs->getName());
            if (0 == relation)
            {
                relation = compare(lhs->getType(), rhs->getType());
                if (0 == relation)
                    relation = compare(lhs->getGroup(), rhs->getGroup());
            }
            return relation;
        }
        int compare(const char* lhs, const char* rhs) const
        {
            if (lhs == rhs)
                return 0;
            if (nullptr == lhs)
                return 1;
            if (nullptr == rhs)
                return -1;
            return stricmp(lhs, rhs);
        }
    };
    using Variants = std::set<Variant, VariantComparator>;
#endif
public:
    CLogAgentFilter(const Variants& variants, LogAgentFilterMode mode, LogAgentFilterType type, const char* pattern);
    ~CLogAgentFilter();

    IMPLEMENT_IINTERFACE;

    // ILogAgentFilter
    LogAgentFilterMode getMode() const override { return m_mode; }
    LogAgentFilterType getType() const override { return m_type; }
    const char*        getPattern() const override { return m_pattern.get(); }
    ILogAgentFilter*   refine(LogAgentFilterMode mode = LAFM_Inclusive, LogAgentFilterType type = LAFT_Unfiltered, const char* pattern = nullptr) override;
    void               reset() override;
    bool               includes(Variant agent, bool recurse = true) const override;

protected:
    virtual bool isMatch(const char* value, const char* pattern) const;
    virtual bool doMatch(bool matched, Variant variant, LogAgentFilterMode mode);
    bool contains(const Variants& variants, const char* name, const char* type, const char* group) const;

protected:
    LogAgentFilterMode m_mode;
    LogAgentFilterType m_type;
    StringAttr         m_pattern;
    Filters            m_refinements;
    Variants           m_variants;
};

class CLogAgentState : implements ILogAgentState, extends CLogAgentFilter, extends CPersistent
{
public:
    CLogAgentState(const ILoggingManager* manager);
    ~CLogAgentState();

    // ILogAgentFilter
    LogAgentFilterMode getMode() const override { return CLogAgentFilter::getMode(); }
    LogAgentFilterType getType() const override { return CLogAgentFilter::getType(); }
    const char*        getPattern() const override { return CLogAgentFilter::getPattern(); }
    ILogAgentFilter*   refine(LogAgentFilterMode mode = LAFM_Inclusive, LogAgentFilterType type = LAFT_Unfiltered, const char* pattern = nullptr) override { return CLogAgentFilter::refine(mode, type, pattern); }
    void               reset() override { CLogAgentFilter::reset(); }
    bool               includes(const ILogAgentVariant* variant, bool recurse = true) const override { return CLogAgentFilter::includes(variant, recurse); }

    // IPersistent
    IMPLEMENT_IPERSISTENT_COLLECTION("LogAgentState", "variants");

protected:
    using Strings = std::set<Owned<String> >;
    static Variants transform(const ILoggingManager* manager);
};

} // namespace EsdlScript

#endif // _EsdlScriptCoreLogging_IPP_
