/*##############################################################################

    HPCC SYSTEMS software Copyright (C) 2024 HPCC Systems®.

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

#pragma once

#include "seclib.hpp"
#include "jscm.hpp"
#include "jthread.hpp"
#include "jtrace.hpp"

/**
 * @brief Templated utility class to decorate an object with additional functionality.
 *
 * A decorator implements the same named interface as the objects it decorates. This class extends
 * the interface, leaving implementation to subclasses. This class requires the decorated object to
 * implement a named interface, even if only IInterface.
 *
 * In the ideal situation, the decorated object's interface is described completely by a named
 * interface. By implementing the same interface, a decorator is interchangeable with the object
 * it decoratos.
 *
 * In less than ideal situations, the decorated object's interface is an extension of a named
 * interface. The decorator extends the extends the named interface, with subclasses required to
 * implement both the named interface and all extensions. The decorator should then be
 * interchangeable with its decorated object as a templated argument, but not be cast to the
 * decorated type.
 *
 * The less ideal scenario is suported by two template parameters. The ideal situation requires
 * only the first.
 * - decorated_t is the type of the object to be decorated. If the the decorated object conforms
 *   to an interface, use of the interface is preferred.
 * - secorated_interface_t is the interface implemented by the decorated object. If not the same
 *   as decorated_t, it is assumed to be a base of that type.
 *
 * Consider the example of ISecManager, generally, and the special case of CLdapSecManager. For
 * most security managers, both template parameters may be ISecManager. CLdapSecManager is an
 * exception because it exposes additional interfaces not included in ISecManager or any other
 * interface. In this case, decorated_t should be CLdapSecManager and decorated_interface_t should
 * be ISecManager.
 */
template <typename decorated_t, typename decorated_interface_t = decorated_t>
class TDecorator : public CInterfaceOf<decorated_interface_t>
{
protected:
    Linked<decorated_t> decorated;
public:
    TDecorator(decorated_t& _decorated) : decorated(&_decorated) {}
    virtual ~TDecorator() {}
    decorated_t* queryDecorated() { return decorated.get(); }
    decorated_t* getDecorated() { return decorated.getLink(); }
};

/**
 * @brief Macro used start tracing a block of code in the security manager decorator.
 *
 * Create a new named client span and enter a try block. Used with END_SEC_MANAGER_TRACE_BLOCK,
 * provides consistent timing and exception handling for the inned code block.
 */
#define START_SEC_MANAGER_TRACE_BLOCK(name) \
    OwnedSpanScope spanScope(queryThreadedActiveSpan()->createClientSpan(name)); \
    try \
    {

/**
 * @brief Macro used to end tracing a block of code in the security manager decorator.
 *
 * Ends a try block and defines standard exception handling. Use with START_SEC_MANAGER_TRACE_BLOCK,
 * provides consistent timing and exception handling for the timed code block.
 */
#define END_SEC_MANAGER_TRACE_BLOCK \
    } \
    catch (IException* e) \
    { \
        spanScope->recordException(e); \
        throw; \
    } \
    catch (...) \
    { \
        Owned<IException> e(makeStringException(-1, "unknown exception")); \
        spanScope->recordException(e); \
        throw; \
    }

/**
 * @brief Decorator for ISecManager that adds tracing to the interface.
 *
 * Tracing is added to selected methods of ISecManager. Traced methods create a span for the
 * duration of the decorated method. Untraced methods are passed through to the decorated object.
 *
 * The default decorated type is sufficient for most security managers. CLdapSecManager is an
 * exception because platform processes depend on interfaces not included in the default.
 * - If ISecManager and CLdapSecManager interfaces are standardized, this point becomes moot.
 * - If ISecManager is extended to declare LDAP-specific interfaces, and CLdapSecManager implements
 *   this interface, create a subclass of TSecManagerTraceDecorator to decorate the new interface
 *   and replace ISecManager with the new interface name as template parameters.
 * - If nothing else changes, create a subclasses of TSecManagerTraceDecorator, with template
 *   parameters CLdapSecManager and ISecManager, to decorate the LDAP-specific interfaces.
 */
template <typename secmgr_t = ISecManager, typename secmgr_interface_t = ISecManager>
class TSecManagerTraceDecorator : public TDecorator<secmgr_t, secmgr_interface_t>
{
    using TDecorator<secmgr_t, secmgr_interface_t>::decorated;
public:
    virtual SecFeatureSet queryFeatures(SecFeatureSupportLevel level) const override
    {
        return decorated->queryFeatures(level);
    }
    virtual ISecUser * createUser(const char * user_name, IEspSecureContext* secureContext = nullptr) override
    {
        return decorated->createUser(user_name, secureContext);
    }
    virtual ISecResourceList * createResourceList(const char * rlname, IEspSecureContext* secureContext = nullptr) override
    {
        return decorated->createResourceList(rlname, secureContext);
    }
    virtual bool subscribe(ISecAuthenticEvents & events, IEspSecureContext* secureContext = nullptr) override
    {
        return decorated->subscribe(events, secureContext);
    }
    virtual bool unsubscribe(ISecAuthenticEvents & events, IEspSecureContext* secureContext = nullptr) override
    {
        return decorated->unsubscribe(events, secureContext);
    }
    virtual bool authorize(ISecUser & user, ISecResourceList * resources, IEspSecureContext* secureContext = nullptr) override
    {
        START_SEC_MANAGER_TRACE_BLOCK("security.authorize");
            return decorated->authorize(user, resources, secureContext);
        END_SEC_MANAGER_TRACE_BLOCK
    }
    virtual bool authorizeEx(SecResourceType rtype, ISecUser & user, ISecResourceList * resources, IEspSecureContext* secureContext = nullptr) override
    {
        START_SEC_MANAGER_TRACE_BLOCK("security.authorize_ex");
            return decorated->authorizeEx(rtype, user, resources, secureContext);
        END_SEC_MANAGER_TRACE_BLOCK
    }
    virtual SecAccessFlags authorizeEx(SecResourceType rtype, ISecUser & user, const char * resourcename, IEspSecureContext* secureContext = nullptr) override
    {
        START_SEC_MANAGER_TRACE_BLOCK("security.authorize_ex");
            return decorated->authorizeEx(rtype, user, resourcename, secureContext);
        END_SEC_MANAGER_TRACE_BLOCK
    }
    virtual SecAccessFlags getAccessFlagsEx(SecResourceType rtype, ISecUser & user, const char * resourcename, IEspSecureContext* secureContext = nullptr) override
    {
        START_SEC_MANAGER_TRACE_BLOCK("security.get_access_flags_ex");
            return decorated->getAccessFlagsEx(rtype, user, resourcename, secureContext);
        END_SEC_MANAGER_TRACE_BLOCK
    }
    virtual SecAccessFlags authorizeFileScope(ISecUser & user, const char * filescope, IEspSecureContext* secureContext = nullptr) override
    {
        START_SEC_MANAGER_TRACE_BLOCK("security.authorize_file_scope");
            return decorated->authorizeFileScope(user, filescope, secureContext);
        END_SEC_MANAGER_TRACE_BLOCK
    }
    virtual bool authorizeFileScope(ISecUser & user, ISecResourceList * resources, IEspSecureContext* secureContext = nullptr) override
    {
        START_SEC_MANAGER_TRACE_BLOCK("security.authorize_file_scope");
            return decorated->authorizeFileScope(user, resources, secureContext);
        END_SEC_MANAGER_TRACE_BLOCK
    }
    virtual bool addResources(ISecUser & user, ISecResourceList * resources, IEspSecureContext* secureContext = nullptr) override
    {
        START_SEC_MANAGER_TRACE_BLOCK("security.add_resources");
            return decorated->addResources(user, resources, secureContext);
        END_SEC_MANAGER_TRACE_BLOCK
    }
    virtual bool addResourcesEx(SecResourceType rtype, ISecUser & user, ISecResourceList * resources, SecPermissionType ptype, const char * basedn, IEspSecureContext* secureContext = nullptr) override
    {
        START_SEC_MANAGER_TRACE_BLOCK("security.add_resources_ex");
            return decorated->addResourcesEx(rtype, user, resources, ptype, basedn, secureContext);
        END_SEC_MANAGER_TRACE_BLOCK
    }
    virtual bool addResourceEx(SecResourceType rtype, ISecUser & user, const char * resourcename, SecPermissionType ptype, const char * basedn, IEspSecureContext* secureContext = nullptr) override
    {
        START_SEC_MANAGER_TRACE_BLOCK("security.add_resource_ex");
            return decorated->addResourceEx(rtype, user, resourcename, ptype, basedn, secureContext);
        END_SEC_MANAGER_TRACE_BLOCK
    }
    virtual bool getResources(SecResourceType rtype, const char * basedn, IResourceArray & resources, IEspSecureContext* secureContext = nullptr) override
    {
        START_SEC_MANAGER_TRACE_BLOCK("security.get_resources");
            return decorated->getResources(rtype, basedn, resources, secureContext);
        END_SEC_MANAGER_TRACE_BLOCK
    }
    virtual bool updateResources(ISecUser & user, ISecResourceList * resources, IEspSecureContext* secureContext = nullptr) override
    {
        START_SEC_MANAGER_TRACE_BLOCK("security.update_resources");
            return decorated->updateResources(user, resources, secureContext);
        END_SEC_MANAGER_TRACE_BLOCK
    }
    virtual bool updateSettings(ISecUser & user, ISecPropertyList * resources, IEspSecureContext* secureContext = nullptr) override
    {
        START_SEC_MANAGER_TRACE_BLOCK("security.update_settings");
            return decorated->updateSettings(user, resources, secureContext);
        END_SEC_MANAGER_TRACE_BLOCK
    }
    virtual bool addUser(ISecUser & user, IEspSecureContext* secureContext = nullptr) override
    {
        START_SEC_MANAGER_TRACE_BLOCK("security.add_user");
            return decorated->addUser(user, secureContext);
        END_SEC_MANAGER_TRACE_BLOCK
    }
    virtual ISecUser * findUser(const char * username, IEspSecureContext* secureContext = nullptr) override
    {
        START_SEC_MANAGER_TRACE_BLOCK("security.find_user");
            return decorated->findUser(username, secureContext);
        END_SEC_MANAGER_TRACE_BLOCK
    }
    virtual ISecUser * lookupUser(unsigned uid, IEspSecureContext* secureContext = nullptr) override
    {
        START_SEC_MANAGER_TRACE_BLOCK("security.lookup_user");
            return decorated->lookupUser(uid, secureContext);
        END_SEC_MANAGER_TRACE_BLOCK
    }
    virtual ISecUserIterator * getAllUsers(IEspSecureContext* secureContext = nullptr) override
    {
        START_SEC_MANAGER_TRACE_BLOCK("security.get_all_users");
            return decorated->getAllUsers(secureContext);
        END_SEC_MANAGER_TRACE_BLOCK
    }
    virtual void getAllGroups(StringArray & groups, StringArray & managedBy, StringArray & descriptions, IEspSecureContext* secureContext = nullptr) override
    {
        START_SEC_MANAGER_TRACE_BLOCK("security.get_all_groups");
            decorated->getAllGroups(groups, managedBy, descriptions, secureContext);
        END_SEC_MANAGER_TRACE_BLOCK
    }
    virtual bool updateUserPassword(ISecUser & user, const char * newPassword, const char* currPassword = nullptr, IEspSecureContext* secureContext = nullptr) override
    {
        START_SEC_MANAGER_TRACE_BLOCK("security.update_user_password");
            return decorated->updateUserPassword(user, newPassword, currPassword, secureContext);
        END_SEC_MANAGER_TRACE_BLOCK
    }
    virtual bool initUser(ISecUser & user, IEspSecureContext* secureContext = nullptr) override
    {
        return decorated->initUser(user, secureContext);
    }
    virtual void setExtraParam(const char * name, const char * value, IEspSecureContext* secureContext = nullptr) override
    {
        decorated->setExtraParam(name, value, secureContext);
    }
    virtual IAuthMap * createAuthMap(IPropertyTree * authconfig, IEspSecureContext* secureContext = nullptr) override
    {
        return decorated->createAuthMap(authconfig, secureContext);
    }
    virtual IAuthMap * createFeatureMap(IPropertyTree * authconfig, IEspSecureContext* secureContext = nullptr) override
    {
        return decorated->createFeatureMap(authconfig, secureContext);
    }
    virtual IAuthMap * createSettingMap(IPropertyTree * authconfig, IEspSecureContext* secureContext = nullptr) override
    {
        return decorated->createSettingMap(authconfig, secureContext);
    }
    virtual void deleteResource(SecResourceType rtype, const char * name, const char * basedn, IEspSecureContext* secureContext = nullptr) override
    {
        decorated->deleteResource(rtype, name, basedn, secureContext);
    }
    virtual void renameResource(SecResourceType rtype, const char * oldname, const char * newname, const char * basedn, IEspSecureContext* secureContext = nullptr) override
    {
        decorated->renameResource(rtype, oldname, newname, basedn, secureContext);
    }
    virtual void copyResource(SecResourceType rtype, const char * oldname, const char * newname, const char * basedn, IEspSecureContext* secureContext = nullptr) override
    {
        decorated->copyResource(rtype, oldname, newname, basedn, secureContext);
    }
    virtual void cacheSwitch(SecResourceType rtype, bool on, IEspSecureContext* secureContext = nullptr) override
    {
        decorated->cacheSwitch(rtype, on, secureContext);
    }
    virtual bool authTypeRequired(SecResourceType rtype, IEspSecureContext* secureContext = nullptr) override
    {
        return decorated->authTypeRequired(rtype, secureContext);
    }
    virtual SecAccessFlags authorizeWorkunitScope(ISecUser & user, const char * filescope, IEspSecureContext* secureContext = nullptr) override
    {
        START_SEC_MANAGER_TRACE_BLOCK("security.authorize_work_unit_scope");
            return decorated->authorizeWorkunitScope(user, filescope, secureContext);
        END_SEC_MANAGER_TRACE_BLOCK
    }
    virtual bool authorizeWorkunitScope(ISecUser & user, ISecResourceList * resources, IEspSecureContext* secureContext = nullptr) override
    {
        START_SEC_MANAGER_TRACE_BLOCK("security.authorize_work_unit_scope");
            return decorated->authorizeWorkunitScope(user, resources, secureContext);
        END_SEC_MANAGER_TRACE_BLOCK
    }
    virtual const char * getDescription() override
    {
        return decorated->getDescription();
    }
    virtual unsigned getPasswordExpirationWarningDays(IEspSecureContext* secureContext = nullptr) override
    {
        return decorated->getPasswordExpirationWarningDays(secureContext);
    }
    virtual aindex_t getManagedScopeTree(SecResourceType rtype, const char * basedn, IArrayOf<ISecResource>& scopes, IEspSecureContext* secureContext = nullptr) override
    {
        return decorated->getManagedScopeTree(rtype, basedn, scopes, secureContext);
    }
    virtual SecAccessFlags queryDefaultPermission(ISecUser& user, IEspSecureContext* secureContext = nullptr) override
    {
        return decorated->queryDefaultPermission(user, secureContext);
    }
    virtual bool clearPermissionsCache(ISecUser & user, IEspSecureContext* secureContext = nullptr) override
    {
        return decorated->clearPermissionsCache(user, secureContext);
    }
    virtual bool authenticateUser(ISecUser & user, bool * superUser, IEspSecureContext* secureContext = nullptr) override
    {
        START_SEC_MANAGER_TRACE_BLOCK("security.authenticate_user");
            return decorated->authenticateUser(user, superUser, secureContext);
        END_SEC_MANAGER_TRACE_BLOCK
    }
    virtual secManagerType querySecMgrType() override
    {
        return decorated->querySecMgrType();
    }
    virtual const char* querySecMgrTypeName() override
    {
        return decorated->querySecMgrTypeName();
    }
    virtual bool logoutUser(ISecUser & user, IEspSecureContext* secureContext = nullptr) override
    {
        START_SEC_MANAGER_TRACE_BLOCK("security.logout_user");
            return decorated->logoutUser(user, secureContext);
        END_SEC_MANAGER_TRACE_BLOCK
    }
    virtual bool retrieveUserData(ISecUser& requestedUser, ISecUser* requestingUser = nullptr, IEspSecureContext* secureContext = nullptr) override
    {
        START_SEC_MANAGER_TRACE_BLOCK("security.retrieve_user_data");
            return decorated->retrieveUserData(requestedUser, requestingUser, secureContext);
        END_SEC_MANAGER_TRACE_BLOCK
    }
    virtual bool removeResources(ISecUser & user, ISecResourceList * resources, IEspSecureContext* secureContext = nullptr) override
    {
        START_SEC_MANAGER_TRACE_BLOCK("security.remove_resources");
            return decorated->removeResources(user, resources, secureContext);
        END_SEC_MANAGER_TRACE_BLOCK
    }
public:
    TSecManagerTraceDecorator(secmgr_t& _decorated)
        : TDecorator<secmgr_t, secmgr_interface_t>(_decorated)
    {
    }
    virtual ~TSecManagerTraceDecorator()
    {
    }
};

using CSecManagerTraceDecorator = TSecManagerTraceDecorator<>;
