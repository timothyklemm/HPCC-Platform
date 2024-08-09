import{_ as e,c as t,o as r,V as a}from"./chunks/framework.gBlNPWt_.js";const f=JSON.parse('{"title":"Security Configuration","description":"","frontmatter":{},"headers":[],"relativePath":"devdoc/SecurityConfig.md","filePath":"devdoc/SecurityConfig.md","lastUpdated":1723205458000}'),o={name:"devdoc/SecurityConfig.md"},n=a('<h1 id="security-configuration" tabindex="-1">Security Configuration <a class="header-anchor" href="#security-configuration" aria-label="Permalink to &quot;Security Configuration&quot;">​</a></h1><p>This document covers security configuration values and meanings. It does not serve as the source for how to configure security, but rather what the different values mean. These are not covered in the docs nor does any reasonable help information exist in the config manager or yaml files.</p><h2 id="supported-configurations" tabindex="-1">Supported Configurations <a class="header-anchor" href="#supported-configurations" aria-label="Permalink to &quot;Supported Configurations&quot;">​</a></h2><p>Security is configured either through an LDAP server or a plugin. Additionally, these are supported in both legacy deployments that use <em>environment.xml</em> and containerized deployments using Kubernetes and Helm charts. While these methods differ, the configuration values remain the same. Focus is placed on the different values and not the deployment target. Differences based on deployment can be found in the relevant platform documents.</p><h2 id="security-managers" tabindex="-1">Security Managers <a class="header-anchor" href="#security-managers" aria-label="Permalink to &quot;Security Managers&quot;">​</a></h2><p>Security is implemented via a security manager interface. Managers are loaded and used by components within the system to check authorization and authentication. LDAP is an exception to the loadable manager model. It is not a compliant loadable module like other security plugins. For that reason, the configuration for each is separated into two sections below: LDAP and Plugin Security Managers.</p><h3 id="ldap" tabindex="-1">LDAP <a class="header-anchor" href="#ldap" aria-label="Permalink to &quot;LDAP&quot;">​</a></h3><p>LDAP is a protocol that connects to an Active Directory server (AD). The term LDAP is used interchangeably with AD. Below are the configuration values for an LDAP connection. These are valid for both legacy (environment.xml) and containerized deployments. For legacy deployments the configuration manager is the primary vehicle for setting these values. However, some values are not available through the tool and must be set manually in the environment.xml if needed for a legacy deployment.</p><p>In containerized environments, a LDAP configuration block is required for each component. Currently, this results in a verbose configuration where much of the information is repeated.</p><p>LDAP is capable if handling user authentication and feature access authorization (such as filescopes).</p><table><thead><tr><th>Value</th><th>Example</th><th>Meaning</th></tr></thead><tbody><tr><td>adminGroupName</td><td>HPCCAdmins</td><td>Group name containing admin users for the AD</td></tr><tr><td>cacheTimeout</td><td>60</td><td>Timeout in minutes to keep cached security data</td></tr><tr><td>ldapCipherSuite</td><td>N/A</td><td>Used when AD is not up to date with latest SSL libs. <br> AD admin must provide</td></tr><tr><td>ldapPort</td><td>389 (default)</td><td>Insecure port</td></tr><tr><td>ldapSecurePort</td><td>636 (default)</td><td>Secure port over TLS</td></tr><tr><td>ldapProtocol</td><td>ldap</td><td><strong>ldap</strong> for insecure (default), using ldapPort<br> <strong>ldaps</strong> for secure using ldapSecurePort</td></tr><tr><td>ldapTimeoutSec</td><td>60 (default 5 for debug, 60 otherwise)</td><td>Connection timeout to an AD before rollint to next AD</td></tr><tr><td>serverType</td><td>ActiveDirectory</td><td>Identifies the type of AD server. (2)</td></tr><tr><td>filesBasedn</td><td>ou=files,ou=ecl_kr,DC=z0lpf,DC=onmicrosoft,DC=com</td><td>DN where filescopes are stored</td></tr><tr><td>groupsBasedn</td><td>ou=groups,ou=ecl_kr,DC=z0lpf,DC=onmicrosoft,DC=com</td><td>DN where groups are stored</td></tr><tr><td>modulesBaseDn</td><td>ou=modules,ou=ecl_kr,DC=z0lpf,DC=onmicrosoft,DC=com</td><td>DN where permissions for resource are stored (1)</td></tr><tr><td>systemBasedn</td><td>OU=AADDC Users,DC=z0lpf,DC=onmicrosoft,DC=com</td><td>DN where the system user is stored</td></tr><tr><td>usersBasedn</td><td>OU=AADDC Users,DC=z0lpf,DC=onmicrosoft,DC=com</td><td>DN where users are stored (3)</td></tr><tr><td>systemUser</td><td>hpccAdmin</td><td>Appears to only be used for IPlanet type ADs, but may still be required</td></tr><tr><td>systemCommonName</td><td>hpccAdmin</td><td>AD username of user to proxy all AD operations</td></tr><tr><td>systemPassword</td><td>System user password</td><td>AD user password</td></tr><tr><td>ldapAdminSecretKey</td><td>none</td><td>Key for Kubernetes secrets (4) (5)</td></tr><tr><td>ldapAdminVaultId</td><td>none</td><td>Vault ID used to load system username and password (5)</td></tr><tr><td>ldapDomain</td><td>none</td><td>Appears to be a comma separated version of the AD domain name components (5)</td></tr><tr><td>ldapAddress</td><td>192.168.10.42</td><td>IP address to the AD</td></tr><tr><td>commonBasedn</td><td>DC=z0lpf,DC=onmicrosoft,DC=com</td><td>Overrides the domain retrieved from the AD for the system user (5)</td></tr><tr><td>templateName</td><td>none</td><td>Template used when adding resources (5)</td></tr><tr><td>authMethod</td><td>none</td><td>Not sure yet</td></tr></tbody></table><p>Notes:</p><ol><li><em>modulesBaseDn</em> is the same as <em>resourcesBaseDn</em> The code looks for first for <em>modulesBaseDn</em> and if not found will search for <em>resourcesBaseDn</em></li><li>Allowed values for <em>serverType</em> are <strong>ActiveDirectory</strong>, <strong>AzureActiveDirectory</strong>, <strong>389DirectoryServer</strong>, <strong>OpenLDAP</strong>, <strong>Fedora389</strong></li><li>For AzureAD, users are managed from the AD dashboard, not via ECLWatch or through LDAP</li><li>If present, <em>ldapAdminVaultId</em> is read and <em>systemCommonName</em> and <em>systemPassword</em> are read from the Kubernetes secrets store and not from the LDAP config values</li><li>Must be configured manually in the environment.xml in legacy environments</li></ol><h3 id="plugin-security-managers" tabindex="-1">Plugin Security Managers <a class="header-anchor" href="#plugin-security-managers" aria-label="Permalink to &quot;Plugin Security Managers&quot;">​</a></h3><p>Plugin security managers are separate shared objects loaded and initialized by the system. The manager interface is passed to components in order to provide necessary security functions. Each plugin has its own configuration. HPCC components can be configured to use a plugin as needed.</p><h4 id="httpasswd-security-manager" tabindex="-1">httpasswd Security Manager <a class="header-anchor" href="#httpasswd-security-manager" aria-label="Permalink to &quot;httpasswd Security Manager&quot;">​</a></h4><p>See documentation for the settings and how to enable.</p><h4 id="single-user-security-manager" tabindex="-1">Single User Security Manager <a class="header-anchor" href="#single-user-security-manager" aria-label="Permalink to &quot;Single User Security Manager&quot;">​</a></h4><p>To be added.</p><h4 id="jwt-security-manager" tabindex="-1">JWT Security Manager <a class="header-anchor" href="#jwt-security-manager" aria-label="Permalink to &quot;JWT Security Manager&quot;">​</a></h4><p>To be added</p>',21),d=[n];function s(i,u,c,l,m,h){return r(),t("div",null,d)}const g=e(o,[["render",s]]);export{f as __pageData,g as default};
