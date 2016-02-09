/*##############################################################################

    HPCC SYSTEMS software Copyright (C) 2012 HPCC Systems®.

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

#pragma warning (disable : 4786)
#pragma warning (disable : 4018)
#pragma warning (disable : 4146)
#pragma warning (disable : 4275)

#ifdef _WIN32
#define AXA_API __declspec(dllexport)
#endif
//#include "ctconnection.h"
#include "basesecurity.hpp"
#include "jmd5.hpp"

#define     cacheTimeout 30000
//#ifdef _WIN32

CBaseSecurityManager::CBaseSecurityManager(const char *serviceName, const char *config)
{
    Owned<IPropertyTree> cfg = createPTreeFromXMLString(config, ipt_caseInsensitive);
    if(cfg.get() == NULL)
        throw MakeStringException(-1, "createPTreeFromXMLString() failed for %s", config);
    init(serviceName,cfg);
    cfg->Release();
}

CBaseSecurityManager::CBaseSecurityManager(const char *serviceName, IPropertyTree *config)
{
    m_dbpasswordEncoding = SecPwEnc_unknown;
    init(serviceName,config);
}

void CBaseSecurityManager::init(const char *serviceName, IPropertyTree *config)
{
	setCredentialServerAvailable(true);

    if(config == NULL)
        return;
    
    m_config.set(config);

    m_permissionsCache.setCacheTimeout( 60 * config->getPropInt("@cacheTimeout", 5) );
    

    m_dbserver.appendf("%s",config->queryProp("@serverName"));
    m_dbuser.appendf("%s",config->queryProp("@systemUser"));
    if(config->hasProp("@ConnectionPoolSize"))
        m_poolsize = atoi(config->queryProp("@connectionPoolSize"));
    else
        m_poolsize = 2;

    StringBuffer encodedPass,encryptedPass;
    encodedPass.appendf("%s",config->queryProp("@systemPassword"));
    decrypt(m_dbpassword, encodedPass.str());

    m_dbpasswordEncoding = SecPwEnc_plain_text;

    StringBuffer strPasswordEncoding;
    const char* encodingType = config->queryProp("@encodePassword");
    if(encodingType && strcmp(encodingType,"MD5") == 0)
        m_dbpasswordEncoding=SecPwEnc_salt_md5;
    else if (encodingType && strcmp(encodingType,"Rijndael") == 0)
        m_dbpasswordEncoding=SecPwEnc_Rijndael;
    else if (encodingType && strcmp(encodingType,"Accurint MD5") == 0)
        m_dbpasswordEncoding = SecPwEnc_salt_accurint_md5;

    if(m_dbserver.length() == 0 || m_dbuser.length() == 0)
        throw MakeStringException(-1, "CBaseSecurityManager() - db server or user is missing");

    IPropertyTree* pNonRestrictedIPTree = config->queryBranch("SafeIPList");
    if(pNonRestrictedIPTree)
    {
        Owned<IPropertyTreeIterator> Itr = pNonRestrictedIPTree->getElements("ip");
        for(Itr->first();Itr->isValid();Itr->next())
        {
            IPropertyTree& tree = Itr->query();
            m_safeIPList[tree.queryProp("")]=true;
        }
    }

    m_passwordExpirationWarningDays = config->getPropInt(".//@passwordExpirationWarningDays", 10); //Default to 10 days

	m_credentialServerReviveInterval = config->getPropInt("@credentialServerReviveInterval", 10) * 1000; // convert config seconds to milliseconds
	m_allowGhostCredentials = config->getPropBool("@allowGhostCredentials", false);
}

CBaseSecurityManager::~CBaseSecurityManager()
{
    MapStrToUsers::iterator pos;
    for(pos=m_userList.begin();pos!=m_userList.end();){
        pos->second->Release();
        pos++;
    }
    dbDisconnect();
}




//interface ISecManager : extends IInterface
ISecUser * CBaseSecurityManager::createUser(const char * user_name)
{
    return (new CSecureUser(user_name, NULL));
}

ISecResourceList * CBaseSecurityManager::createResourceList(const char * rlname)
{
    return (new CSecurityResourceList(rlname));
}

bool CBaseSecurityManager::subscribe(ISecAuthenticEvents & events)
{
    m_subscriber.set(&events);
    return true;
}

bool CBaseSecurityManager::unsubscribe(ISecAuthenticEvents & events)
{
    if (&events == m_subscriber.get())
    {
        m_subscriber.set(NULL);
    }
    return true;
}

bool CBaseSecurityManager::authorize(ISecUser & sec_user, ISecResourceList * Resources, IEspContext* ctx)
{   
	if (!isCredentialServerAvailable())
	{
		setCredentialServerAvailable(reviveCredentialServer());
		m_permissionsCache.setGhostCredentialsActive(ghostCredentialsAreActive());
	}

    if(sec_user.getAuthenticateStatus() != AS_AUTHENTICATED)
    {
        bool bOk = ValidateUser(sec_user);
        if(bOk == false)
            return false;
    }
    return ValidateResources(sec_user,Resources);
}

bool CBaseSecurityManager::updateSettings(ISecUser &sec_user, ISecPropertyList* resources)
{
	// Cannot update without without a server connection
	if (!isCredentialServerAvailable())
		return false;

	CSecurityResourceList * reslist = (CSecurityResourceList*)resources;
	if (!reslist)
		return true;
	IArrayOf<ISecResource>& rlist = reslist->getResourceList();
	int nResources = rlist.length();
	if (nResources <= 0)
		return true;

	bool rc = false;
	if (m_permissionsCache.isCacheEnabled() == false)
		return updateSettings(sec_user, rlist);

	bool* cached_found = (bool*)alloca(nResources*sizeof(bool));
	int nFound = m_permissionsCache.lookup(sec_user, rlist, cached_found);
	if (nFound >= nResources)
		return true;

	IArrayOf<ISecResource> rlist2;
	for (int i = 0; i < nResources; i++)
	{
		if (*(cached_found + i) == false)
		{
			ISecResource& secRes = rlist.item(i);
			secRes.Link();
			rlist2.append(secRes);
		}
	}
	rc = updateSettings(sec_user, rlist2);
	if (rc)
		m_permissionsCache.add(sec_user, rlist2);
	return rc;
}

bool CBaseSecurityManager::updateSettings(ISecUser & sec_user, IArrayOf<ISecResource>& rlist)
{
	CSecureUser* user = (CSecureUser*)&sec_user;
	if (user == NULL)
		return false;

	int usernum = findUser(user->getName(), user->getRealm());
	if (usernum < 0)
	{
		PrintLog("User number of %s can't be found", user->getName());
		return false;
	}

	IArrayOf<ISecResource> batchList;
	bool batched = supportBatchedSettings();

	ForEachItemIn(x, rlist)
	{
		CSecurityResource* secRes = QUERYINTERFACE(&rlist.item(x), CSecurityResource);
		if (!secRes)
		{
			continue;
		}

		//AccessFlags default value is -1. Set it to 0 so that the settings can be cached. AccessFlags is not being used for settings.
		secRes->setAccessFlags(0);
		if (batched)
			secRes->setValue("-1");

		// Only resources with "resource" parameters are of interest
		const char* resource = secRes->getParameter("resource");
		if (resource && *resource)
		{
			batchList.append(*secRes);
		}
	}
	
	if (batchList.length() > 0)
	{
		if (batched)
		{
			dbValidateSettings(sec_user, batchList, usernum, user->getRealm());
		}
		else
		{
			ForEachItemIn(y, batchList)
			{
				dbValidateSetting(sec_user, batchList.item(y), usernum, user->getRealm());
			}
		}
	}

	return true;
}


bool CBaseSecurityManager::ValidateResources(ISecUser & sec_user, ISecResourceList * resources, IEspContext* ctx)
{
    CSecurityResourceList * reslist = (CSecurityResourceList*)resources;
    if(!reslist)
        return true;
    IArrayOf<ISecResource>& rlist = reslist->getResourceList();
    int nResources = rlist.length();
    if (nResources <= 0)
        return true;

    bool rc = false;
    if (isCredentialServerAvailable() && m_permissionsCache.isCacheEnabled()==false)
        return ValidateResources(sec_user, rlist);

    bool* cached_found = (bool*)alloca(nResources*sizeof(bool));
    int nFound = m_permissionsCache.lookup(sec_user, rlist, cached_found);
    if (nFound >= nResources)
    {
        return true;
    }

	// Don't go any further if we're forcing the use of the cache, as the rest of the
	// code reads from the database.
	if (!isCredentialServerAvailable())
		return false;

    IArrayOf<ISecResource> rlist2;
    for (int i=0; i < nResources; i++)
    {
        if (*(cached_found+i) == false)
        {
            ISecResource& secRes = rlist.item(i);
            secRes.Link();
            rlist2.append(secRes);
        }
    }
    rc = ValidateResources(sec_user, rlist2);
    if (rc)
    {
        IArrayOf<ISecResource> rlistValid;
        for (int i=0; i < rlist2.ordinality(); i++)
        {
            ISecResource& secRes = rlist2.item(i);
            if(secRes.getAccessFlags() >= secRes.getRequiredAccessFlags() || secRes.getAccessFlags() == SecAccess_Unknown)
            {
                secRes.Link();
                rlistValid.append(secRes);
            }
        }
        m_permissionsCache.add(sec_user, rlistValid);
    }
        
    return rc;
}

static bool stringDiff(const char* str1, const char* str2)
{
    if(!str1 || !*str1)
    {
        if(!str2 || !*str2)
            return false;
        else
            return true;
    }
    else
    {
        if(!str2 || !*str2)
            return true;
        else
            return (strcmp(str1, str2) != 0);
    }
}

bool CBaseSecurityManager::ValidateUser(ISecUser & sec_user, IEspContext* ctx)
{
	if (ValidateUserInCache(sec_user, ctx))
		return true;

	// Cache validation failed and database is unavailable.
	if (ghostCredentialsAreActive())
	{
		sec_user.setAuthenticateStatus(AS_UNEXPECTED_ERROR);
		return false;
	}

    if(!IsPasswordValid(sec_user))
    {
        ERRLOG("Password validation failed for user: %s",sec_user.getName());
		return false;
    }
    else
    {
        if(IsIPRestricted(sec_user)==true)
        {
            if(ValidateSourceIP(sec_user,m_safeIPList)==false)
            {
                ERRLOG("IP check failed for user:%s coming from %s",sec_user.getName(),sec_user.getPeer());
                sec_user.setAuthenticateStatus(AS_INVALID_CREDENTIALS);
                return false;
            }

			if (m_permissionsCache.isCacheEnabled())
				m_permissionsCache.addAllowedRestrictedIp(sec_user, sec_user.getPeer());
        }
		else
		{
			updateUserRoaming(sec_user);
			if (m_permissionsCache.isCacheEnabled())
				m_permissionsCache.add(sec_user);
		}

        sec_user.setAuthenticateStatus(AS_AUTHENTICATED);
    }
    return true;
}

bool CBaseSecurityManager::ValidateUserInCache(ISecUser & sec_user, IEspContext* ctx)
{
	bool bReturn = false;

	if (m_permissionsCache.isCacheEnabled() && m_permissionsCache.lookup(makeQualifiedUser(sec_user)))
	{
		StringBuffer clientip(sec_user.getPeer());
		if (IsIPRestricted(sec_user) && !m_permissionsCache.isAllowedRestrictedIp(sec_user, clientip.str()))
		{
			//we seem to be coming from a different peer... this is not good
			WARNLOG("Found user %d in cache, but have to re-validate IP, because it was coming from %s but is now coming from %s", sec_user.getUserID(), sec_user.getPeer(), clientip.str());
			sec_user.setAuthenticateStatus(AS_INVALID_CREDENTIALS);
			sec_user.setPeer(clientip.str());
			m_permissionsCache.removeFromUserCache(sec_user);
		}
		else
		{
			sec_user.setAuthenticateStatus(AS_AUTHENTICATED);
			bReturn = true;
		}
	}

	return bReturn;
}

bool CBaseSecurityManager::IsPasswordValid(ISecUser& sec_user)
{
    StringBuffer password(sec_user.credentials().getPassword());
    EncodePassword(password);
    StringBuffer SQLQuery;
    buildAuthenticateQuery(sec_user.getName(),password.str(),sec_user.getRealm(),SQLQuery);
    return dbauthenticate(sec_user , SQLQuery);
}

bool CBaseSecurityManager::IsIPRestricted(ISecUser& sec_user)
{
    const char* iprestricted = sec_user.getProperty("iprestricted");
    if(iprestricted!=NULL && strncmp(iprestricted,"1",1)==0)
        return true;
    return false;
}


void CBaseSecurityManager::EncodePassword(StringBuffer& password)
{
    StringBuffer encodedPassword;
    switch (m_dbpasswordEncoding)
    {
        case SecPwEnc_salt_md5:
            md5_string(password,encodedPassword);
            password.clear().append(encodedPassword.str());
            break;
        case SecPwEnc_Rijndael:
            encrypt(encodedPassword,password.str());
            password.clear().append(encodedPassword.str());
            break;
        case SecPwEnc_salt_accurint_md5:
            password.toUpperCase();
            md5_string(password,encodedPassword);
            password.clear().append(encodedPassword.str());
            break;
    }
}



bool CBaseSecurityManager::addResources(ISecUser & sec_user, ISecResourceList * Resources)
{
    return false;
}
bool CBaseSecurityManager::addUser(ISecUser & user)
{
    return false;
}

int CBaseSecurityManager::getUserID(ISecUser& user)
{
    return findUser(user.getName(),user.getRealm());
}

bool CBaseSecurityManager::ValidateResources(ISecUser & sec_user,IArrayOf<ISecResource>& rlist, IEspContext* ctx)
{
    CSecureUser* user = (CSecureUser*)&sec_user;
    if(user == NULL)
        return false;

    int usernum = findUser(user->getName(),user->getRealm());
    if(usernum < 0)
    {
        PrintLog("User number of %s can't be found", user->getName());
        return false;
    }

    ForEachItemIn(x, rlist)
    {
        ISecResource* res = (ISecResource*)(&(rlist.item(x)));
        if(res == NULL)
            continue;
        dbValidateResource(sec_user, *res,usernum,user->getRealm(), ctx);
    }

    return true;
}

bool CBaseSecurityManager::updateResources(ISecUser & user, ISecResourceList * resources)
{   
    //("CBaseSecurityManager::updateResources");
    if(!resources)
        return false;

    const char* username = user.getName();
    //const char* realm = user.getRealm();
    const char* realm = NULL;

    int usernum = findUser(username,realm);
    if(usernum <= 0)
    {
        PrintLog("User number of %s can't be found", username);
        return false;
    }
    CSecurityResourceList * reslist = (CSecurityResourceList*)resources;
    if (reslist)
    {
        IArrayOf<ISecResource>& rlist = reslist->getResourceList();
        ForEachItemIn(x, rlist)
        {
            ISecResource* res = (ISecResource*)(&(rlist.item(x)));
            if(res == NULL)
                continue;
            dbUpdateResource(*res,usernum,realm);
        }
    }
    return true;
}

bool CBaseSecurityManager::updateUserPassword(ISecUser& user, const char* newPassword, const char* currPassword)
{
    //("CBaseSecurityManager::updateUser");
    if(!newPassword)
        return false;
    StringBuffer password(newPassword);
    EncodePassword(password);
    const char* realm = NULL;
    bool bReturn =  dbUpdatePasswrd(user.getName(),realm,password.str());
    if(bReturn == true)
        user.credentials().setPassword(password.str());
    //need to flush the users info from the cache....
    if(m_permissionsCache.isCacheEnabled())
        m_permissionsCache.removeFromUserCache(user);
    return bReturn;
}


void CBaseSecurityManager::logon_failed(const char* user, const char* msg) 
{
    PrintLog("%s: %s", user, msg);
}

int CBaseSecurityManager::findUser(const char* user,const char* realm)
{
    if(user == NULL)
        return -1;
    synchronized block(m_usermap_mutex);
    int* uidptr = m_usermap.getValue(user);
    if(uidptr != NULL)
    {
        return *uidptr;
    }
    else
    {
        int uid = dbLookupUser(user,realm);
        if(uid >= 0)
        {
            m_usermap.setValue(user, uid);
        }
        return uid;
    }
}



void CBaseSecurityManager::setDefaultDomainName(const char* domainName)
{
	if (domainName && strchr(domainName, ','))
		throw MakeStringException(-1, "default domain name (%s) cannot contain ','", domainName);

	m_defaultDomainName.set(domainName);
}

ISecUser& CBaseSecurityManager::makeQualifiedUser(ISecUser& sec_user) const
{
	const char* username = sec_user.getName();
	StringBuffer qualifiedUsername;
	bool cached = false;

	if (const_cast<CPermissionsCache&>(m_permissionsCache).isCacheEnabled() &&
		!m_permissionsCache.isCached(username) &&
		getQualifiedUsername(username, qualifiedUsername) &&
		m_permissionsCache.isCached(qualifiedUsername))
	{
		sec_user.setName(qualifiedUsername);
	}

	return sec_user;

}

bool CBaseSecurityManager::getQualifiedUsername(const char* username, StringBuffer& qualifiedUsername) const
{
	const char*  defaultDomain = getDefaultDomainName();
	const char*  delimiter = NULL;
	bool         changed = false;

	qualifiedUsername.set(username);

	if (!defaultDomain || !*defaultDomain) // no default domain ==> no qualification required
	{
	}
	else if (!username || !*username) // no username ==> no qualification possible
	{
	}
	else if ((delimiter = strchr(username, '@')) == NULL) // <username> ==> add @<default domain>
	{
		qualifiedUsername.appendf("@%s", defaultDomain);
		changed = true;
	}
	else if (!*(delimiter + 1)) // <username>@ ==> add <default domain>
	{
		qualifiedUsername.append(defaultDomain);
		changed = true;
	}
	else // <username>@<domain> ==> already qualified
	{
	}

	return changed;
}

bool CBaseSecurityManager::updateUserRoaming(ISecUser& sec_user) const
{
	IPList::const_iterator it = m_safeIPList.find(sec_user.getPeer());
	bool result = ((it != m_safeIPList.end()) && (it->second == true));

	if (result)
	{
		sec_user.setPropertyInt("IPRoaming", 0);
	}

	return result;
}

bool CBaseSecurityManager::isUserCached(ISecUser& sec_user) const
{
	CPermissionsCache& cache = const_cast<CPermissionsCache&>(m_permissionsCache);
	return (cache.isCacheEnabled() && cache.lookup(makeQualifiedUser(sec_user)));
}


//#endif //_WIN32
