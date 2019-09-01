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

/**
 * In a default configuration, all log agents registered with a service's log
 * manager are tasked with processing every transaction. There are a number of
 * reasons why a dynamic service may want to limit which agents process which
 * transactions. For example:
 *   * A log agent may require user opt-in, or permit user opt-out.
 *   * A log agent's output may be used to affect future requests, and some
 *     requests may need to be excluded.
 *   * Different methods in a service may need to log to different locations.
 *
 * These interfaces declare the core scripting mechanism that will control
 * which agents are applied on a per transaction basis. By default, all agents
 * are applied to all transactions.
 */

#ifndef _EsdlScriptCoreLogging_HPP_
#define _EsdlScriptCoreLogging_HPP_

#include "es_core_utility.hpp"
#include "loggingagentbase.hpp"

namespace EsdlScript
{
// Assigned to each ILogAgentFilter instance, specifies whether matching agents
// should or should not be enabled for a transaction.
enum LogAgentFilterMode
{
    LAFM_Inclusive,
    LAFM_Exclusive
};

// Assigned to each ILogAgentFilter instance, specifies whether matches are
// made by comparing agent name, type, or group.
enum LogAgentFilterType
{
    LAFT_Unfiltered,
    LAFT_Group,
    LAFT_Type,
    LAFT_Name,
};

using ILogAgentVariant = IEspLogAgentVariant;

//
interface ILogAgentFilter : extends IInterface
{
    // Are matching agents included or excluded?
    virtual LogAgentFilterMode getMode() const = 0;
    inline  bool isInclusive() const { return getMode() == LAFM_Inclusive; }
    inline  bool isExclusive() const { return getMode() == LAFM_Exclusive; }

    // How are agent matches determined?
    virtual LogAgentFilterType getType() const = 0;
    inline  bool isUnfiltered() const { return getType() == LAFT_Unfiltered; }
    inline  bool isByGroup() const { return getType() == LAFT_Group; }
    inline  bool isByType() const { return getType() == LAFT_Type; }
    inline  bool isByName() const { return getType() == LAFT_Name; }

    // What is the applied matching value?
    virtual const char* getPattern() const = 0;

    // Apply an additional constraint on a filtered set of agents. For example,
    // one might refine a filter that includes a group by excluding a type.
    //
    // A filter is best imagined as a tree. Given a filter with two
    // refinements, the filter is the parent node, the refinements are children
    // of the parent and are siblings of each other. Each leaf node in the tree
    // identifies a (possibly empty) set of agents that are enabled. The set of
    // enabled agents is the combination of agents from all leaf nodes.
    virtual ILogAgentFilter* refine(LogAgentFilterMode mode = LAFM_Inclusive, LogAgentFilterType type = LAFT_Unfiltered, const char* pattern = nullptr);
    inline  ILogAgentFilter* includeGroup(const char* pattern) { return refine(LAFM_Inclusive, LAFT_Group, pattern); }
    inline  ILogAgentFilter* excludeGroup(const char* pattern) { return refine(LAFM_Exclusive, LAFT_Group, pattern); }
    inline  ILogAgentFilter* includeType(const char* pattern) { return refine(LAFM_Inclusive, LAFT_Type, pattern); }
    inline  ILogAgentFilter* excludeType(const char* pattern) { return refine(LAFM_Exclusive, LAFT_Type, pattern); }
    inline  ILogAgentFilter* includeName(const char* pattern) { return refine(LAFM_Inclusive, LAFT_Name, pattern); }
    inline  ILogAgentFilter* excludeName(const char* pattern) { return refine(LAFM_Exclusive, LAFT_Name, pattern); }
    virtual void reset() = 0;

    // Tests whether a specific variant is enabled or disabled by a filter.
    // A log agent, having restored a saved agent state, will be expected to
    // test each of its variants for inclusion, and only log that data which
    // is associated with an enabled variant.
    virtual bool includes(const ILogAgentVariant* variant, bool recurse = true) const = 0;
    inline  bool excludes(const ILogAgentVariant* variant, bool recurse = true) const { return !includes(variant, recurse); }
};

// The root filter, including all variants of all manager-defined agents.
interface ILogAgentState : extends ILogAgentFilter, extends IPersistent
{
};

// Utility class providing a standardized interface for accessing an owned
// instance of ILogAgentState.
DECLARE_IOWNER(LogAgentState);

} // namespace EsdlScript


#endif // _EsdlScriptCoreLogging_HPP_
