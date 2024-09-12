# Mock Security Manager

A security manager plugin intended for use by developers to easily evaluate platform handling of verious manager outcomes. For any given input (i.e., user name) the manager provides a pew-determined, or canned, response. Results can be restricted to matching specific user names, or all user names may be accepted.

If all security managers operate by matching input credentials to a defined state, how is this any different?
1. It's self contained. THere is no need to setup and maintain a separate backend, like an LDAP server, nor is it necessary to rely on mainainers of existing backends to configure test cases.
1. Results are reproducible. With no external dependencies for results the results returned for any given user name are predictable.
1. It is simpler. The configuration defines the user and resource states; no computation is necessary. For example, a user can fail authentication for an expired password without defining a password or an expiration date.

As the name suggests, this manager merely pretends to offer security. User names matter oonly to select test cases. Passwords don't matter. It should never be used in a live, production, environment.

Development will occur in stages. The first stage will focus on the needs of ESPs implementing customer business logic. This includes user authentication, user properties, location authorization, feature authorization, and setting updates.

Support for platform logic, such as file scopes and work unit scopes, may follow in subsequent stages.

## Configuration

### Manager

The configuration is focused on the description of a user. This description includes the values to bet set in a secure user instance during authentication, as well as the definitions of permissions granted and settings defined. Multiple users are permitted, and none are required.

The YAML layout of a user resembles:

```yml
user:
- name: conditionally required unique identifier
  status: optional user status enumerated string
  authStatus: optional authenticate status enumerated string
  superUser: optional Boolean flag
  property:
  - name: required unique identifier
    value: optional string
  location:
  - path: required unique location resource path
    access: required access flag enumerated string
  feature:
  - path: required unique feature resource path (i.e., name)
    access: required access flag enumerated string
  setting:
  - path: required unique setting resource path (i.e., name)
    value: required setting resource value string
  ...
```

Use with a bare-metal istallation expexts the XML equivalent to the YAML shown above. The configmgr user interface can only edit `name`, `password`, `status`, `authStatus`, and `superUser`. Manual editing of the `environment.xml` file is required to define additional user content.

### Binding

The manager imposes no new requirements on the binding configuration, which is only used to assemble the location, feature, and setting authmap instances. With respect to these resource definitions, the `resource` attribute has no meaning. The abscence of *backend* data eliminates the need to map a path to a value.

### Points to Ponder

1. Although no other platform managers do it, there are managers that enforce peer IP restrictions. Collections of permissible IP addresses, defined both per user and per manager, may be added. When `authorize` is used to perform authentication, there is no response that conveys peer rejection other than an exception. Should consider extending the `authStatus` enumeration with a valuall other e for this condition, because all other values lead to incorrect error messaging.

1. A password value may not be needed. If a user can be configured to respond with an invalid credentials status, it isn't necessary to apply password checks in order to fail. Passwords in configuration can be avoided without adding external dependencies on K8s secrets or going through jsecrets.

## Behavior

Manager behavior varies based on the number of users defined.

## Without Defined Users

- Any user name and password are accepted for secure user authentication.
- Secure user status and authenticate status are unchanged.
- No secure user properties are defined.
- All location and feature resources are granted full access.
- Setting resources are not updated.

This is like wrapping the `userNameOnly` authentication method in a security manager. Unlike `userNameOnly`, which is not implemented as a security manager, this allows evaluation of code that depends on the use of a security manager.

### With Defined Users

Results depend on which values are included in the definition.

- Secure user authentication:
  - No configured name matches any name input.
  - A configured name requires an exact input name match.
  - Configured names must be unique. Since omission of a name implies an empty name, at most one defined user may omit its name.
- Secure user status is set only when given.
- Secure user authenticate is set only when given.
- Secure user properties are set when given.
- Location resources:
  - No resources grants full access.
  - No matching path sets access to unknown.
  - Matching path grants configured access.
- Feature resources:
  - No resources grants full access.
  - No matching path sets access to unknown.
  - Matching path grants configured access.
- Setting resource value require matching configured paths.

Defining a single user without a name expands on the `userNameOnly` authentication method by enabling additional non-default user states.

Defining a single user with only a name is comparable to using the `SingleUserSecurity` security manager, with the obvious exception that any password is accepted. Defining more properties increases the utility.

Defining multiple users unlocks the full power of the manager. Multiple use cases, including those requiring multiple users, may be avaluated using a common configuration.

## Implementation

Details are TBD.