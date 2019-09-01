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

#include "esdl_script_core.ipp"
#include <xpp/XmlPullParser.h>
#include <algorithm>
#include <climits>
#include <memory>

namespace EsdlScript
{
namespace Tags
{
const char* root = "EsdlScript";
const char* library = "xsdl:Library";
const char* phasePreflight = "xsdl:PreflightPhase";
const char* phaseRequest = "xsdl:RequestPhase";
const char* phaseResponse = "xsdl:ResponsePhase";
const char* phaseLogManager = "xsdl:LogManagerPhase";
const char* phaseLogAgent = "xsdl:LogAgentPhase";

const char* customRequestTransform = "xsdl:CustomRequestTransform";
} // namespace Tags

static const CTraceState::TSFrame defaultTraceStates(
{
        { TraceType_Success, LogMax },
        { TraceType_Warning, LogNormal },
        { TraceType_Error, LogMin },
        { TraceType_CallStack, LogMax },
        { TraceType_UserInfo1, LogNormal },
        { TraceType_UserInfo2, LogNormal },
        { TraceType_UserInfo3, LogNormal },
});

static inline TraceType mapTraceType(OutcomeType type)
{
    switch (type)
    {
    case OutcomeType_Success: return TraceType_Success;
    case OutcomeType_Warning: return TraceType_Warning;
    case OutcomeType_Error:   return TraceType_Error;
    default:                  return TraceType_Undefined;
    }
}

const IPTree* findNamed(const IPTree* node, const char* name)
{
    if (isEmptyString(name))
        throw MakeStringException(-1, "EsdlScript persistence restore error [name cannot be empty]");
    if (nullptr == node)
        return node;
    if (strcmp(node->queryName(), name) == 0)
        return node;
    return node->queryBranch(VStringBuffer("//%s", name));
}

static inline OutcomeType fillStatus(IOutcomes& outcomes, OutcomeType type, int code, const char* message)
{
    outcomes.record(type, code, "%s", message);
    return type;
}
static inline OutcomeType fillStatus(IOutcomes* outcomes, OutcomeType type, int code, const char* message)
{
    return (outcomes != nullptr ? fillStatus(*outcomes, type, code, message) : type);
}

CFactory::CFactory()
{
}

CFactory::~CFactory()
{
}

void CFactory::initialize()
{
    registerStatement(Tags::phasePreflight, [](){ return new CPhaseStatement(Phase_Preflight); });
    registerStatement(Tags::phaseRequest, [](){ return new CPhaseStatement(Phase_Request); });
    registerStatement(Tags::phaseResponse, [](){ return new CPhaseStatement(Phase_Response); });
    registerStatement(Tags::phaseLogManager, [](){ return new CPhaseStatement(Phase_LogManager); });
    registerStatement(Tags::phaseLogAgent, [](){ return new CPhaseStatement(Phase_LogAgent); });
    registerStatement(Tags::customRequestTransform, [](){ return nullptr; });
}

OutcomeType CFactory::registerStatement(const char* tag, Creator creator, IOutcomes* outcomes)
{
    WriteLockBlock block(m_creatorLock);
    auto it = m_creatorMap.find(tag);

    if (it != m_creatorMap.end())
    {
        it->second = creator;
        return fillStatus(outcomes, OutcomeType_Warning, -1, "redefinition of factory tag '%s', tag");
    }
    m_creatorMap[tag] = creator;
    return fillStatus(outcomes, OutcomeType_Success, 0, VStringBuffer("factory tag '%s' defined", tag));
}

bool CFactory::isStatement(const char* name) const
{
    if (isEmptyString(name))
        return false;

    ReadLockBlock block(m_creatorLock);
    auto it = m_creatorMap.find(name);

    return it != m_creatorMap.end();
}

IStatement* CFactory::create(ILoadContext& context, IStatement* parent) const
{
    auto tagName = context.currentTag();
    bool found = false;
    Owned<IStatement> statement;
    auto outcomes = context.queryOutcomes();

    OutcomesContext ohc(*context.queryOutcomes(), "create", "factory");

    if (parent != nullptr && !parent->acceptsChild(tagName))
    {
        outcomes->recordWarning(-1, "'%s' element cannot be a child of '%s'", tagName, parent->queryTag());
        return nullptr;
    }

    { // limit the ReadLockBlock scope
        ReadLockBlock block(m_creatorLock);
        auto it = m_creatorMap.find(tagName);

        if (it != m_creatorMap.end())
        {
            found = true;
            statement.setown((it->second)());
        }
    }

    if (!found)
        outcomes->recordWarning(-1, "'%s' element not a defined statement", tagName);
    else if (statement.get() == nullptr)
        outcomes->recordError(-1, "'%s' element construction failed", tagName);
    else if (!statement->acceptsParent(parent))
        outcomes->recordWarning(-1, "'%s' statement cannot be a child of '%s'", tagName, parent->queryTag());
    else
        statement->initialize(context);
    return statement.getClear();
}

OutcomeType CFactory::unregisterStatement(const char* tag, IOutcomes* outcomes)
{
    if (isEmptyString(tag))
        return fillStatus(outcomes, OutcomeType_Error, -1, VStringBuffer("factory tag cannot be empty"));

    WriteLockBlock block(m_creatorLock);
    auto it = m_creatorMap.find(tag);

    if (m_creatorMap.end() == it)
        return fillStatus(outcomes, OutcomeType_Success, -1, VStringBuffer("factory tag '%s' not found", tag));
       m_creatorMap.erase(it);
       return fillStatus(outcomes, OutcomeType_Success, 0, VStringBuffer("factory tag '%s' removed", tag));
}

OutcomeType CLibrary::load(ILoadContext& context)
{
    if (!context.atStartTag() || stricmp(context.currentTag(), Tags::library) != 0)
        throw MakeStringException(-1, "ILoadContext must point to %s to load the library", Tags::library);

    auto outcomes = context.queryOutcomes();
    OutcomesContext ohc(*outcomes, "load", "library");

    while (context.next())
    {
        if (context.atStartTag())
        {
            StringBuffer tagName(context.currentTag());
            Owned<IStatement> statement(context.queryFactory()->create(context));

            if (statement.get() == nullptr)
                outcomes->recordWarning(-1, "'%s' element not did not resolve to statement", tagName.str());
            else if (isEmptyString(statement->queryUID()))
                outcomes->recordWarning(-1, "'%s' statement missing UID; cannot be retained", statement->queryTag());
            else if (!outcomes->isError())
            {
                WriteLockBlock block(m_statementsLock);
                auto it = m_statements.find(statement->queryUID());

                if (it != m_statements.end())
                    outcomes->recordWarning(-1, "'%s' statement '%s' has non-unique UID; cannot be retained", statement->queryTag(), statement->queryUID());
                else
                    m_statements[statement->queryUID()] = statement;
            }
        }
    }

    return *outcomes;
}

const IStatement* CLibrary::queryStatement(const char* uid) const
{
    if (isEmptyString(uid))
        return nullptr;

    ReadLockBlock block(m_statementsLock);
    auto it = m_statements.find(uid);

    if (it != m_statements.end())
        return it->second.get();
    return nullptr;
}

const IStatement* CLibrary::getStatement(const char* uid) const
{
    if (isEmptyString(uid))
        return nullptr;

    ReadLockBlock block(m_statementsLock);
    auto it = m_statements.find(uid);

    if (it != m_statements.end())
        return it->second.getLink();
    return nullptr;
}



bool defaultParentAcceptor(const CStatement* self, const IStatement* statement)
{
    return true;
}

bool defaultChildAcceptor(const CStatement* self, const char* name)
{
    return true;
}

bool defaultExtensionAcceptor(const CStatement* self, const char* name)
{
    return true;
}

uint8_t defaultChildPredicate(const Owned<IStatement>&, IProcessContext*)
{
    return CStatement::ChildPredicate_Match | CStatement::ChildPredicate_Continue;
}



CStatement::CStatement()
    : m_parentAcceptor(defaultParentAcceptor)
    , m_childAcceptor(defaultChildAcceptor)
    , m_extensionAcceptor(defaultExtensionAcceptor)
    , m_childPredicate(defaultChildPredicate)
{
}

CStatement::~CStatement()
{
}

bool CStatement::initialize(ILoadContext& context)
{
    OutcomesContext ohc(context.queryOutcomes());

    if (!isNew())
    {
        (*ohc).recordError(-1, "'%s' statement invalid internal state (expected %d, found %d)", context.currentTag(), InitializationState_New, m_initializationState);
    }
    else if (!ohc.isError())
    {
        m_initializationState = InitializationState_InProgress;
        m_tag.append(context.currentTag());
        m_uid.append(context.currentAttribute("uid"));

        if (initializeSelf(context))
        {
            while (!ohc.isError() && context.next())
            {
                switch (context.currentEntityType())
                {
                case XmlPullParser::START_TAG:
                    handleStartTag(context);
                    break;

                case XmlPullParser::CONTENT:
                    handleContent(context);
                    break;

                case XmlPullParser::END_TAG:
                    handleEndTag(context);
                    break;

                default:
                    break;
                }
            }

            if (!ohc.isError())
                validateSelf(context);
        }
    }

    return !(*ohc).isError();
}

bool CStatement::initializeSelf(ILoadContext& context)
{
    return true;
}

void CStatement::acceptChild(IStatement* child)
{
    m_children.push_back(child);
}
void CStatement::handleStartTag(ILoadContext& context)
{
    OutcomesContext ohc(context.queryOutcomes());

    auto tagName = context.currentTag();

    if (context.queryFactory()->isStatement(tagName))
    {
        if (acceptsChild(tagName))
        {
            Owned<IStatement> child(context.queryFactory()->create(context, this));

            if (child.get() != nullptr && !(*ohc).isError())
            {
                auto subChild = dynamic_cast<CStatement*>(child.get());

                if (subChild != nullptr)
                    subChild->m_parent = this;
                else
                    (*ohc).recordWarning(-1, "unable to set parent of '%s' statement - not derived from CStatement", queryTag(), child->queryTag());
                acceptChild(child);
                (*ohc).recordSuccess(-1, "'%s' statement added child '%s'", queryTag(), child->queryTag());
            }
            else
            {
                // factory is expected to have logged error conditions
            }
        }
        else
        {
            (*ohc).recordWarning(-1, "'%s' statement cannot be a child of '%s' statement", tagName, queryTag());
            context.skip();
        }
    }
    else if (acceptsExtension(tagName))
    {
        extendSelf(context);
    }
    else
    {
        (*ohc).recordWarning(-1, "'%s' statement contains unrecognized element '%s'", queryTag(), tagName);
        context.skip();
    }
}

void CStatement::handleContent(ILoadContext& context)
{
    if (acceptsContent())
    {
        extendSelf(context);
    }
}

void CStatement::handleEndTag(ILoadContext& context)
{
}

void CStatement::extendSelf(ILoadContext& context)
{
    OutcomesContext ohc(context.queryOutcomes());
    switch (context.currentEntityType())
    {
    case XmlPullParser::START_TAG:
        (*ohc).recordWarning(-1, "'%s' statement '%s' ignored extension element '%s'", queryTag(), queryUID(), context.currentTag());
        context.skip();
        break;

    case XmlPullParser::CONTENT:
        if (!context.isCurrentContentSpace())
            (*ohc).recordWarning(-1, "'%s' statement '%s' ignored content '%s'", queryTag(), (isEmptyString(queryUID()) ? "N/A" : queryUID()), context.currentContent());
        break;
    }
}

void CStatement::validateSelf(ILoadContext& context)
{
}

bool CStatement::acceptsChildren() const
{
    return m_acceptsChildren;
}

bool CStatement::acceptsContent() const
{
    return m_acceptsContent;
}

bool CStatement::acceptsExtensions() const
{
    return m_acceptsExtensions;
}

bool CStatement::acceptsParent(const IStatement* parent) const
{
    return m_parentAcceptor(this, parent);
}

bool CStatement::acceptsChild(const char* name) const
{
    return acceptsChildren() && !isEmptyString(name) && m_childAcceptor(this, name);
}

bool CStatement::acceptsExtension(const char* name) const
{
    return acceptsExtensions() && !isEmptyString(name) && m_extensionAcceptor(this, name);
}

void CStatement::checkPhase(ILoadContext& context) const
{
    // Only a limited number of statements are expected to have phase restrictions.
    // Do nothing with this node and check children.

    for (auto& c : m_children)
    {
        c->checkPhase(context);
    }
}

bool CStatement::isChild(const IStatement* statement) const
{
    return find(statement) != m_children.end();
}

CStatement::Children::iterator CStatement::find(const IStatement* child)
{
    return std::find_if(m_children.begin(), m_children.end(), [&](const Owned<IStatement>& item) {
        return item.get() == child;
    });
}

CStatement::Children::const_iterator CStatement::find(const IStatement* child) const
{
    return std::find_if(m_children.begin(), m_children.end(), [&](const Owned<IStatement>& item) {
        return item.get() == child;
    });
}

void CStatement::setParentAcceptor(ParentAcceptor acceptor)
{
    m_parentAcceptor = acceptor ? acceptor : defaultParentAcceptor;
}

void CStatement::setChildAcceptor(ChildAcceptor acceptor)
{
    m_childAcceptor = acceptor ? acceptor : defaultChildAcceptor;
}

void CStatement::setExtensionAcceptor(ExtensionAcceptor acceptor)
{
    m_extensionAcceptor = acceptor ? acceptor : defaultExtensionAcceptor;
}

void CStatement::setChildPredicate(ChildPredicate predicate)
{
    m_childPredicate = predicate ? predicate : defaultChildPredicate;
}

void CStatement::setAcceptsChildren(bool accepts)
{
    m_acceptsChildren = accepts;
}

void CStatement::setAcceptsContent(bool accepts)
{
    m_acceptsContent = accepts;
}

void CStatement::setAcceptsExtensions(bool accepts)
{
    m_acceptsExtensions = accepts;
}

const char* CStatement::queryReadCursor() const
{
    return nullptr;
}

const char* CStatement::queryWriteCursor() const
{
    return nullptr;
}

void CStatement::process(IProcessContext& context, IParentContext* parentInfo, const char* readXPath, const char* writeXPath) const
{
    // native code programming error --> throw exception
    if (!isInitialized())
        throw MakeStringException(-1, "EsdlScript statement process error ['%s' statement '%s' is uninitialized]", queryTag(), (queryUID() != nullptr ? queryUID() : "N/A"));

    IReadCursor::StFrame rFrame(*context.queryReadCursor(), *this);
    IWriteCursor::StFrame wFrame(*context.queryWriteCursor(), *this);
    Owned<IParentContext> selfInfo;

    if (rFrame.isValid() && wFrame.isValid() &&
        processSelf(context, parentInfo, readXPath, writeXPath, selfInfo))
    {
        processChildren(context, selfInfo, readXPath, writeXPath);
    }
}

bool CStatement::processSelf(IProcessContext& context, IParentContext* parentInfo, const char* readXPath, const char* writeXPath, Owned<IParentContext>& selfInfo) const
{
    return true;
}

void CStatement::processChildren(IProcessContext& context, IParentContext* selfInfo, const char* readXPath, const char* writeXPath) const
{
    if (acceptsChildren() && !m_children.empty())
    {
        for (auto& child : m_children)
        {
            auto predicateResponse = m_childPredicate(child, &context, selfInfo);

            if (predicateResponse & ChildPredicate_Match)
            {
                child->process(context, selfInfo, readXPath, writeXPath);
            }

            if (predicateResponse & ChildPredicate_Stop)
            {
                break;
            }
        }
    }
}

bool CStatement::isEvaluable() const
{
    return false;
}

bool CStatement::evaluate(IProcessContext* context, IParentContext* parentInfo) const
{
    return false;
}

bool CStatement::isComparable() const
{
    return false;
}

bool CStatement::compare(const char* value, IProcessContext* context) const
{
    return false;
}

const char* CStatement::queryTag() const
{
    return m_tag;
}

const char* CStatement::queryUID() const
{
    return m_uid;
}

Phase CStatement::queryPhase() const
{
    if (m_parent != nullptr)
        return m_parent->queryPhase();
    return Phase_Unknown;
}

// -------------------------------------------------------------------------------------------------

bool Script::add(const IPTree* tree, const char* xpath, const char* service, const char* method)
{
    if (nullptr == tree)
        throw MakeStringException(-1, "EsdlScript environment initialization error [invalid parameter]");

    ScriptFragment fragment;
    bool added = false;

    if (isEmptyString(xpath))
    {
        if (tree->hasChildren())
            toXML(tree, fragment.content);
        else
        {
            Owned<IAttributeIterator> attrs = tree->getAttributes();
            auto content = tree->queryProp(".");

            fragment.content << '<' << tree->queryName();
               ForEach(*attrs)
                fragment.content << ' ' << attrs->queryName() << "=\"" << attrs->queryValue() << '"';
               if (isEmptyString(content))
                   fragment.content << "/>";
               else
                   fragment.content << '>' << content << "</" << tree->queryName() << '>';
        }
        fragment.service = service;
        fragment.method = method;
        push_back(fragment);
        added = true;
    }
    else
    {
        Owned<IPTreeIterator> nodes = tree->getElements(xpath);

        ForEach(*nodes)
        {
            added = add(&nodes->query(), "", service, method) || added;
        }
    }

    return added;
}

CEnvironment::CEnvironment()
    : CLibraryOwner([](){ return new CLibrary(); })
    , CFactoryOwner([](){ return new CFactory(); })
{
}

CEnvironment::~CEnvironment()
{
}

ILoadContext* CEnvironment::createLoadContext() const
{
    Owned<CLoadContext> context(new CLoadContext());

    context->m_parser.reset(new XmlPullParser());
    context->m_parser->setSupportNamespaces(true);
    context->m_factory.set(queryFactory());

    return context.getClear();
}

OutcomeType CEnvironment::load(const IPTree* binding, const char* service)
{
    Owned<ILoadContext> context = createLoadContext();

    if (context.get() != nullptr)
    {
        context->addServiceConstraint(service);
        load(*context.get(), binding);
        return *context->queryOutcomes();
    }
    return OutcomeType_Error;
}

void CEnvironment::load(ILoadContext& context, const IPTree* binding)
{
    static VStringBuffer crtPaths[2] =
    {
        VStringBuffer("%s", Tags::customRequestTransform),
        VStringBuffer("Transforms/%s", Tags::customRequestTransform)
    };

    if (nullptr == binding)
        throw MakeStringException(-1, "EsdlScript environment initialization error [invalid parameter]");

    Owned<IPTreeIterator> definitions(binding->getElements("Definition"));
    ForEach(*definitions)
    {
        auto node = &definitions->query();
        auto esdlService = node->queryProp("@esdlService");

        if (!context.isServiceConstraint(esdlService))
            continue;

        Script script;

        script.add(node, Tags::root, esdlService);
        node = node->queryBranch("Methods");
        if (nullptr == node)
            throw MakeStringException(-1, "EsdlScript environment initialization error [invalid binding definition]");

        Owned<IPTreeIterator> methods = node->getElements("Method");
        ForEach(*methods)
        {
            auto method = &methods->query();
            auto name = method->queryName();

            if (!script.add(method, Tags::root, esdlService, name))
            {
                Script subscript;

                if (subscript.add(method, crtPaths[0], esdlService) ||
                    subscript.add(method, crtPaths[1], esdlService) ||
                    subscript.add(method, crtPaths[0], esdlService, name) ||
                    subscript.add(method, crtPaths[1], esdlService, name))
                {
                    // Wrap all legacy markup fragments in:
                    //   1. EsdlScript - make the fragment loadable
                    //   2. xsdl:RequestPhase - legacy markup only applies to request processing

                    ScriptFragment fragment;

                    fragment.content << '<' << Tags::root << "><" << Tags::phaseRequest << '>';
                    for (auto& sf : subscript)
                        fragment.content << sf.content;
                    fragment.content << "</" << Tags::phaseRequest << "></" << Tags::root << '>';
                    fragment.service = esdlService;
                    fragment.method = name;
                    script.push_back(fragment);
                }
            }
        }

        load(context, script);
    }
}

OutcomeType CEnvironment::load(const Script& script, const char* service)
{
    Owned<ILoadContext> context(createLoadContext());

    if (context.get() != nullptr)
    {
        context->addServiceConstraint(service);
        load(*context.get(), script);
        return *context->queryOutcomes();
    }
    return OutcomeType_Error;
}

void CEnvironment::load(ILoadContext& context, const Script& script)
{
    for (auto& fragment : script)
    {
        if (context.isServiceConstraint(fragment.service))
            load(context, fragment);
    }

    checkSyntax(context);
}

void CEnvironment::load(ILoadContext& context, const ScriptFragment& fragment)
{
    if (isEmptyString(fragment.content))
        throw MakeStringException(-1, "EsdlScript environment initialization error [missing script fragment content]");
    if (isEmptyString(fragment.service))
        throw MakeStringException(-1, "EsdlScript environment initialization error [missing script fragment service]");

    context.setInput(fragment.content);
    context.setCurrentService(fragment.service);
    context.setCurrentMethod(fragment.method);

    while (context.next())
    {
        if (context.atStartTag() && strieq(context.currentTag(), Tags::root))
            loadScript(context);
    }

    context.setCurrentMethod(nullptr);
    checkSyntax(context);
}

static Phase mapPhase(const char* tag)
{
    auto result = Phase_Unknown;

    if (stricmp(tag, Tags::phasePreflight))
        result = Phase_Preflight;
    else if (stricmp(tag, Tags::phaseRequest))
        result = Phase_Request;
    else if (stricmp(tag, Tags::phaseResponse))
        result = Phase_Response;
    else if (stricmp(tag, Tags::phaseLogManager))
        result = Phase_LogManager;
    else if (stricmp(tag, Tags::phaseLogAgent))
        result = Phase_LogAgent;

    return result;
}

void CEnvironment::loadScript(ILoadContext& context)
{
    Phase phase = Phase_Unknown;

    while (context.next())
    {
        if (context.atStartTag())
        {
            auto tagName = context.currentTag();
            if (stricmp(tagName, "Library"))
                loadLibrary(context);
            else if ((phase = mapPhase(tagName)) != Phase_Unknown)
                loadPhase(context, phase);
            else
            {
                context.queryOutcomes()->recordWarning(-1, "skipping unexpected script element '%s'", tagName);
                context.skip();
            }
            break;
        }
    }
}

void CEnvironment::loadLibrary(ILoadContext& context)
{
    Owned<ILibrary> library(getLibrary());

    if (library.get() != nullptr)
        library->load(context);
    else
        throw MakeStringException(-1, "EsdlScript environment initialize error [no library to load definitions]");
}

void CEnvironment::loadPhase(ILoadContext& context, Phase phase)
{
    OutcomesContext ohc(context.queryOutcomes());
    auto tagName = context.currentTag();

    if (isEmptyString(context.currentMethod()))
    {
        (*ohc).recordError(-1, "unexpected phase statement '%s' outside a method definition", tagName);
        return;
    }

    WriteLockBlock sblock(m_serviceMapLock);
    auto& methodMap = m_serviceMap[context.currentService()];
    //WriteLockBlock mblock[methodMap.second];
    auto& phaseMap = methodMap.first[context.currentMethod()];
    //WriteLockBlock pblock[phaseMap.second];
    auto& statement = phaseMap.first[phase];

    if (statement.get() != nullptr)
    {
        (*ohc).recordWarning(-1, "ignoring extra '%s' statement for method '%s'", tagName, context.currentMethod());
        context.skip();
        return ;
    }

    statement.setown(context.queryFactory()->create(context));
    if (nullptr == statement.get() || !(*ohc).isError())
    {
        // do nothing
    }
    else if (statement->queryPhase() != phase)
    {
        (*ohc).recordError(-1, "'%s' statement is phase %d (%d expected)", statement->queryTag(), statement->queryPhase());
    }
}

IProcessContext* CEnvironment::createProcessContext(IEspContext& espContext, const char* service, const char* method) const
{
    return nullptr;
}

IProcessContext* CEnvironment::restoreProcessContext(const char* logXML) const
{
    return nullptr;
}

PhaseResult CEnvironment::processPhase(IProcessContext& context, Phase phase) const
{
    OutcomesContext ohc(context.queryOutcomes());
    PhaseResult result = PhaseResult_Unknown;
    ReadLockBlock sblock(m_serviceMapLock);

    auto sit = m_serviceMap.find(context.queryCurrentService());

    if (m_serviceMap.end() == sit)
        return PhaseResult_Success;

    ReadLockBlock mblock(sit->second.second);
    auto mit = sit->second.first.find(context.queryCurrentMethod());

    if (sit->second.first.end() == mit)
        return PhaseResult_Success;

    ReadLockBlock pblock(mit->second.second);
    auto pit = mit->second.first.find(context.queryCurrentPhase());

    if (mit->second.first.end() == pit)
        return PhaseResult_Success;

    try
    {
        pit->second->process(context, nullptr, nullptr); // TODO: extract initial R/W roots
        result = PhaseResult_Success; // TODO: base result on outcomes status
    }
    catch (Abort& a)
    {
        result = PhaseResult_Aborted;
    }
    catch (Fail& f)
    {
        result = PhaseResult_Failed;
    }
    catch (IException* e)
    {
        StringBuffer msg;
        const char*  audience = LogMsgAudienceToVarString(e->errorAudience());

        if (stricmp(audience, "UNKNOWN") == 0)
            audience = nullptr;
        (*ohc).recordError(-1, "%s%sexception [%d| %s]", (audience != nullptr ? audience : ""), (audience != nullptr ? " " : ""), e->errorCode(), e->errorMessage(msg).str());
        e->errorMessage(msg);
        e->Release();
        result = PhaseResult_AbnormalTermination;
    }
    catch (std::exception& e)
    {
        (*ohc).recordError(-1, "exception [%s]", e.what());
        result = PhaseResult_AbnormalTermination;
    }
    catch (...)
    {
        (*ohc).recordError(-1, "unknown exception");
        result = PhaseResult_AbnormalTermination;
    }

    return result;
}

void CEnvironment::checkSyntax(ILoadContext& context) const
{
    ReadLockBlock sblock(m_serviceMapLock);
    for (auto& sit : m_serviceMap)
    {
        ReadLockBlock mblock(sit.second.second);
        for (auto& mit : sit.second.first)
        {
            ReadLockBlock pblock(mit.second.second);
            for (auto& pit : mit.second.first)
            {
                pit.second->checkPhase(context);
            }
        }
    }
}

// -------------------------------------------------------------------------------------------------

COutcomes::COutcome::COutcome(String* component, String* operation, OutcomeType type, int code, const char* message)
    : COutcomeTypeContainer(type)
    , m_code(code)
    , m_message(message)
{
    m_component.set(component);
    m_operation.set(operation);
}

COutcomes::COutcome::~COutcome()
{
}

StringBuffer& COutcomes::COutcome::toString(StringBuffer& buffer) const
{
    buffer << "EsdlScript ";
    if (m_component.get() != nullptr)
        buffer << m_component->str() << ' ';
    if (m_operation.get() != nullptr)
        buffer << m_operation->str() << ' ';
    buffer << typeString() << ' ';
    buffer << '[';
    buffer << m_code;
    if (!m_message.isEmpty())
        buffer << '|' << m_message;
    buffer << ']';

    return buffer;
}

StringBuffer& COutcomes::COutcome::persist(StringBuffer& xml) const
{
    xml.append("<Outcome");
    if (m_component.get() != nullptr)
        xml.appendf(" component=\"%s\"", m_component->str());
    if (m_operation.get() != nullptr)
        xml.appendf(" operation=\"%s\"", m_operation->str());
    xml.appendf(" type=\"%d\" type_text=\"%s\"", m_type, typeString());
    xml.appendf(" code=\"%d\"", m_code);
    if (m_message.isEmpty())
        xml.append("/>");
    else
        xml.appendf("><![CDATA[%s]]></Outcome>", m_message.get());
    return xml;
}

COutcomes::COutcomeIterator::COutcomeIterator(const COutcomes& outcomes)
    : m_outcomes(outcomes)
{
    m_outcomesIt = m_outcomes.m_history.end();
}

COutcomes::COutcomeIterator::~COutcomeIterator()
{
}

bool COutcomes::COutcomeIterator::first()
{
    m_outcomesIt = m_outcomes.m_history.begin();
    return isValid();
}

bool COutcomes::COutcomeIterator::next()
{
    if (isValid())
        m_outcomesIt++;
    return isValid();
}

bool COutcomes::COutcomeIterator::isValid()
{
    return m_outcomesIt != m_outcomes.m_history.end();
}

const IOutcome& COutcomes::COutcomeIterator::query()
{
    return *m_outcomesIt->get();
}

#define RECORD_IMPL(t) \
    StringBuffer msg; \
    if (!isEmptyString(format)) \
    { \
        va_list      args; \
        va_start(args, format); \
        msg.valist_appendf(format, args); \
    } \
    record(createOutcome(m_component, m_operation, t, code, msg))

void COutcomes::record(OutcomeType type, int code, const char* format, ...)
{
    RECORD_IMPL(type);
}
void COutcomes::recordSuccess(int code, const char* format, ...)
{
    RECORD_IMPL(OutcomeType_Success);
}
void COutcomes::recordWarning(int code, const char* format, ...)
{
    RECORD_IMPL(OutcomeType_Warning);
}
void COutcomes::recordError(int code, const char* format, ...)
{
    RECORD_IMPL(OutcomeType_Error);
}

IOutcomeIterator* COutcomes::getOutcomes() const
{
    return new COutcomeIterator(*this);
}

StringBuffer& COutcomes::persist(StringBuffer& xml) const
{
    xml.appendf("<Outcomes count=\"%zu\" overall_type=\"%d\" overall_type_text=\"%s\">", m_history.size(), m_type, typeString());
    for (auto& h : m_history)
        h->persist(xml);
    xml.append("</Outcomes>");

    return xml;
}

void COutcomes::record(IOutcome* outcome)
{
    if (outcome != nullptr)
    {
        if (outcome->getType() > getType())
            m_type = outcome->getType();

        m_history.push_back(outcome);
    }
}

IOutcome* COutcomes::createOutcome(String* component, String* operation, OutcomeType type, int code, const char* message) const
{
    return new COutcome(component, operation, type, code, message);
}

// -------------------------------------------------------------------------------------------------

static const TEnumMapper<TraceType> TraceTypeMapper({
    { TraceType_Success, "success" },
    { TraceType_Warning, "warning" },
    { TraceType_Error, "error" },
    { TraceType_CallStack, "call_stack" },
    { TraceType_UserInfo1, "user_1" },
    { TraceType_UserInfo2, "user_2" },
    { TraceType_UserInfo3, "user_3" },
    { TraceType_Undefined, "undefined" },
});

const char* mapTraceType(TraceType type)
{
    return TraceTypeMapper.mapping(type);
}

TraceType mapTraceType(const char* label)
{
    return TraceTypeMapper.mapping(label);
}

CTraceState::CTraceState()
{
    pushTraceStateFrame();
}

CTraceState::~CTraceState()
{
}

StringBuffer& CTraceState::persist(StringBuffer& xml) const
{
    size_t remainingFrameCount = m_tsStack.size();

    xml.appendf("<TraceState frames=\"%zu\">", remainingFrameCount);
    for (auto& f : m_tsStack)
    {
        if (f.empty())
            continue;

        xml.appendf("<Frame index=\"%zu\">", m_tsStack.size() - remainingFrameCount);
        for (auto& s : f)
        {
            auto label = mapTraceType(s.first);
            if (nullptr == label)
                continue;

            xml.appendf("<State type=\"%s\" level=\"%d\"/>", label, s.second);
        }
        xml.append("</Frame>");
        remainingFrameCount--;
    }
    xml.append("</TraceState>");
    return xml;
}

void CTraceState::restore(const IPTree* node)
{
    node = findNamed(node, "TraceState");
    if (node != nullptr)
    {
        TSStack tmp;
        auto count = node->getPropInt("@frames");

        if (count <= 0)
            throw MakeStringException(-1, "EsdlScript trace restore [invalid frame count]");

        Owned<IPTreeIterator> frames = node->getElements("Frame");

        tmp.resize(TSStack::size_type(count), TSFrame());
        ForEach(*frames)
        {
            auto& frame = frames->query();
            auto  idx = frame.getPropInt("@index");

            if (idx < 0 || idx >= count)
                throw MakeStringException(-1, "EsdlScript trace restore [invalid frame index %d]", idx);

            Owned<IPTreeIterator> states = frame.getElements("State");

            ForEach(*states)
            {
                auto& state = states->query();
                auto  type = mapTraceType(state.queryProp("@type"));
                auto  level = state.getPropInt("@level");

                if (type != TraceType_Undefined)
                    tmp[idx][type] = level;
            }
        }
        m_tsStack = tmp;
    }
}

void CTraceState::pushTraceStateFrame()
{
    m_tsStack.push_back(TSFrame());
}

void CTraceState::setLogLevel(TraceType type, LogLevel level, TraceFrame inFrame)
{
    size_t idx = (TraceFrame_Current == inFrame ? m_tsStack.size() - 1 : inFrame);
    if (idx >= m_tsStack.size())
        throw MakeStringException(-1, "EsdlScript trace level error ['%zu' is an invalid stack frame value]", idx);
    m_tsStack[idx][type] = level;
}

LogLevel CTraceState::getLogLevel(TraceType type) const
{
    for (auto fit = m_tsStack.rbegin(); fit != m_tsStack.rend(); ++fit)
    {
        auto sit = fit->find(type);

        if (sit != fit->end())
            return sit->second;
    }
    return LogNone;
}

void CTraceState::popTraceStateFrame()
{
    if (m_tsStack.size() <= 1)
        throw MakeStringException(-1, "EsdlScript trace state error [unable to pop last frame]");
    m_tsStack.pop_back();
}

StringBuffer& CTraceState::toString(StringBuffer& buffer) const
{
    bool first = true;

    for (auto& e : TraceTypeMapper.m_mapping)
    {
        if (!first)
            buffer << ", ", first = false;
        buffer << e.second << '=' << getLogLevel(e.first);
    }
    return buffer;
}

// -------------------------------------------------------------------------------------------------

static const TEnumMapper<VariableState> VariableStateMapper({
    { VariableState_Declared, "declared" },
    { VariableState_Defined, "defined" },
    { VariableState_Undefined, "undefined" },
});

const char* mapVariableState(VariableState state)
{
    return VariableStateMapper.mapping(state);
}

VariableState mapVariableState(const char* label)
{
    return VariableStateMapper.mapping(label);
}

CVariables::CVariable::CVariable(VariableState state, const char* name, const char* value, VariableFrame frame)
    : m_state(state)
    , m_frame(frame)
    , m_name(name)
    , m_value(value)
{
    if (VariableState_Undefined == m_state || m_name.isEmpty() || m_value.isEmpty())
    {
        StringBuffer xml;
        throw MakeStringException(-1, "EsdlScript variable definition error [invalid variable definition '%s']", persist(xml).str());
    }
}

StringBuffer& CVariables::CVariable::persist(StringBuffer& xml) const
{
    xml << "<Variable name=\"" << m_name << "\" value=\"" << m_value << "\" state=\"" << mapVariableState(m_state) << "\" frame=\"" << m_frame << "\"/>";
    return xml;
}

CVariables::CVariableIterator::CVariableIterator(const CVariables& stack, VariableFrame maxFrame, VariableFrame minFrame)
    : m_stack(stack)
    , m_maxFrame(maxFrame)
    , m_minFrame(minFrame)
{
    if (CURRENT_VARIABLE_FRAME == m_minFrame)
        m_minFrame = frameCount() - 1;
    if (CURRENT_VARIABLE_FRAME == m_maxFrame)
        m_maxFrame = frameCount() - 1;
}

CVariables::CVariableIterator::~CVariableIterator()
{
}

bool CVariables::CVariableIterator::first()
{
    if (m_maxFrame >= frameCount() || m_minFrame > m_maxFrame)
    {
        return false;
    }

    m_curFrame = m_maxFrame;
    m_varIt = m_stack.m_data[m_curFrame].begin();
    return (isValid() || next());
}

bool CVariables::CVariableIterator::next()
{
    if (CURRENT_VARIABLE_FRAME == m_curFrame)
        return false;

    if (m_varIt == m_stack.m_data[m_curFrame].end())
    {
        if (m_curFrame > m_minFrame)
            m_varIt = m_stack.m_data[--m_curFrame].begin();
        else
            return false;
    }
    else
        m_varIt++;
    return (isValid() || next());
}

bool CVariables::CVariableIterator::isValid()
{
    return (m_curFrame != CURRENT_VARIABLE_FRAME && m_varIt != m_stack.m_data[m_curFrame].end());
}

const IVariable& CVariables::CVariableIterator::query()
{
    static Owned<IVariable> invalidVariable;

    if (isValid())
        return *(m_varIt->get());
    else
        return *invalidVariable.get();
}

CVariables::CVariables()
{
    pushVariablesFrame();
}

CVariables::~CVariables()
{
}

StringBuffer& CVariables::persist(StringBuffer& xml) const
{
    VariableFrame idx = 0;

    xml.appendf("<Variables frames=\"%zu\">", m_data.size());
    for (auto& varSet : m_data)
    {
        xml.appendf("<Frame depth=\"%u\" entries=\"%zu\">", idx, varSet.size());
        for (auto v : varSet)
        {
            v->persist(xml);
        }
        xml.append("</Frame>");
    }
    xml.append("</Variables>");

    return xml;
}

void CVariables::restore(const IPTree* node)
{
    node = findNamed(node, "Variables");
    if (node != nullptr)
    {
        auto count = node->getPropInt("@frames");

        if (count <= 0)
            throw MakeStringException(-1, "EsdlScript variables restore error [invalid frame count %d]", count);

        Owned<IPTreeIterator> frames = node->getElements("Frame");
        VariableSetStack tmp;

        tmp.resize(VariableSetStack::size_type(count), VariableSet());
        ForEach(*frames)
        {
            auto& frame = frames->query();
            auto  idx = frame.getPropInt("@depth");

            if (idx < 0 || idx >= tmp.size())
                throw MakeStringException(-1, "EsdlScript variables restore error [invalid frame depth %d]", idx);

            Owned<IPTreeIterator> vars = frame.getElements("Variable");

            ForEach(*vars)
            {
                auto& var = vars->query();
                auto  name = var.queryProp("@name");
                auto  value = var.queryProp("@value");
                auto  state = mapVariableState(var.queryProp("@state"));
                auto  frame = var.getPropInt("@frame");

                if (isEmptyString(name) || isEmptyString(value) || VariableState_Undefined == state)
                    continue;
                if (frame != idx)
                    ; // TODO: log inconsistency?

                addVariable(state, name, value, VariableFrame(frame));
            }
        }
        m_data = tmp;
    }
}

IVariableListener* CVariables::getVariableListener() const
{
    return m_listener;
}

void CVariables::setVariableListener(IVariableListener* listener)
{
    m_listener = listener;
}

VariableFrame CVariables::pushVariablesFrame()
{
    m_data.push_back(VariableSet());
    return VariableFrame(m_data.size());
}

OutcomeType CVariables::addVariable(VariableState state, const char* name, const char* value, VariableFrame atFrame, IOutcomes* outcomes)
{
    if (CURRENT_VARIABLE_FRAME == atFrame)
        atFrame = m_data.size() - 1;

    if (atFrame >= m_data.size())
        return OutcomeType_Error;
    if (state != VariableState_Declared && state != VariableState_Defined)
        return OutcomeType_Error;
    if (isEmptyString(name) or isEmptyString(value))
        return OutcomeType_Error;

    auto& frame = m_data[atFrame];
    Owned<IMutableVariable> needle(createVariable(state, name, value, atFrame));
    auto it = frame.find(needle);

    if (it != frame.end())
    {
        auto& match = *it;
        if (match->isDeclared() && needle->isDefined())
        {
            // A declared variable can always transition to defined.
            match->updateState(state);
            if (stricmp(match->queryValue(), needle->queryValue()) != 0)
            {
                match->updateValue(needle->queryValue());
                applyVariableUpdate(name, queryVariable(name, CURRENT_VARIABLE_FRAME, atFrame));
            }
        }
        else if (match->isDefined() && needle->isDeclared())
        {
            // A defined variable can be redeclared without error.
        }
        else if (match->isDeclared() && needle->isDeclared())
        {
            // Should re-declaration be allowed?
        }
        else if (match->isDefined() && needle->isDefined())
        {
            // Variable re-definition is not allowed;
            return OutcomeType_Error;
        }
        else
        {
            // Unexpected scenario
            return OutcomeType_Error;
        }
    }
    else
    {
        // new variable is always allowed
        auto newerValue = (atFrame == m_data.size() - 1 ? nullptr : queryVariable(name, CURRENT_VARIABLE_FRAME, atFrame + 1));
        auto olderValue = (nullptr == newerValue && atFrame > 0 ? queryVariable(name, atFrame - 1) : nullptr);
        auto currentValue = (newerValue != nullptr ? newerValue : olderValue);

        frame.insert(needle);
        if (nullptr == currentValue || stricmp(value, currentValue) != 0)
        {
            applyVariableUpdate(name, value);
        }
    }

    return OutcomeType_Success;
}

bool CVariables::isVariable(const char* name, VariableFrame maxFrame, VariableFrame minFrame) const
{
    return queryVariable(name, maxFrame, minFrame) != nullptr;
}

const char* CVariables::queryVariable(const char* name, VariableFrame maxFrame, VariableFrame minFrame) const
{
    if (CURRENT_VARIABLE_FRAME == maxFrame)
        maxFrame = m_data.size() - 1;
    if (CURRENT_VARIABLE_FRAME == minFrame)
        minFrame = m_data.size() -1;

    if (minFrame > maxFrame || maxFrame >= m_data.size())
        return nullptr;

    Owned<IMutableVariable> needle(createVariable(VariableState_Undefined, name, "."));

    for (auto idx = maxFrame; idx >= minFrame; idx--)
    {
        needle->updateFrame(idx);

        auto  it = m_data[idx].find(needle);

        if (it != m_data[idx].end())
            return (*it)->queryValue();
    }

    return nullptr;
}

void CVariables::popVariablesFrame()
{
    if (m_data.size() <= 1)
        throw MakeStringException(-1, "EsdlScript variable stack error [cannot pop base frame]");

    auto& frame = m_data.back();

    for (auto& v : frame)
        applyVariableUpdate(v->queryName(), queryVariable(v->queryName(), v->getFrame() - 1));
    m_data.pop_back();
}

IVariableIterator* CVariables::getVariables(VariableFrame maxFrame, VariableFrame minFrame) const
{
    return new CVariableIterator(*this, maxFrame, minFrame);
}

void CVariables::applyVariableUpdate(const char* name, const char* value)
{
    if (m_listener != nullptr)
        m_listener->applyVariableUpdate(name, value);
}

CVariables::IMutableVariable* CVariables::createVariable(VariableState state, const char* name, const char* value, VariableFrame frame) const
{
    return new CVariable(state, name, value, frame);
}


// -------------------------------------------------------------------------------------------------

// -------------------------------------------------------------------------------------------------

CPhaseStatement::CPhaseStatement(Phase phase)
    : m_phase(phase)
{
    if (Phase_Unknown == phase)
        throw MakeStringException(-1, "EsdlScript statement construction error [invalid phase]");
}

CPhaseStatement::~CPhaseStatement()
{
}

Phase CPhaseStatement::queryPhase() const
{
    return m_phase;
}

void CPhaseStatement::checkPhase(ILoadContext& context) const
{
    // Assume statement loading has validated the expected phase and do nothing.

    for (auto& c : m_children)
    {
        c->checkPhase(context);
    }
}

// -------------------------------------------------------------------------------------------------

bool parentIsRoot(const CStatement* self, const IStatement* parent)
{
    return nullptr == parent;
}

bool parentIsChoose(const CStatement* self, const IStatement* parent)
{
    return parent != nullptr && streq(parent->queryTag(), Tags::choose);
}

bool childOfChoose(const CStatement* parent, const char* tag)
{
    if (isEmptyString(tag))
        return false;
    if (strieq(tag, Tags::when))
        return true;
    if (strieq(tag, Tags::otherwise))
        return parent->queryChildTag(tag) == nullptr;
    return false;
}

// -------------------------------------------------------------------------------------------------

uint8_t noChildren(const Owned<IStatement>&, IProcessContext*)
{
    return CStatement::ChildPredicate_NoMatch | CStatement::ChildPredicate_Stop;
}

uint8_t allChildren(const Owned<IStatement>&, IProcessContext*)
{
    return CStatement::ChildPredicate_Match | CStatement::ChildPredicate_Continue;
}

uint8_t evaluatedChildren(const Owned<IStatement>& child, IProcessContext* context)
{
    return CStatement::ChildPredicate_Continue |
           (child->evaluate(context) ?
                   CStatement::ChildPredicate_Match :
                   CStatement::ChildPredicate_NoMatch);
}

uint8_t firstEvaluatedChild(const Owned<IStatement>& child, IProcessContext* context)
{
    return (child->evaluate(context) ?
            CStatement::ChildPredicate_Match | CStatement::ChildPredicate_Stop :
            CStatement::ChildPredicate_NoMatch | CStatement::ChildPredicate_Continue);
}

uint8_t actionableChildren(const Owned<IStatement>& child, IProcessContext* context)
{
    uint8_t result = CStatement::ChildPredicate_Continue;

    if (!child->isEvaluable() || child->evaluate(context))
        result = result | CStatement::ChildPredicate_Match;
    return result;
}

} // namespace EsdlScript

EsdlScript::IEnvironment* createEsdlScriptEnvironment()
{
    return new EsdlScript::CEnvironment();
}

bool operator < (const EsdlScript::IVariable& lhs, const EsdlScript::IVariable& rhs)
{
    if (lhs.getFrame() < rhs.getFrame())
        return true;
    else if (lhs.getFrame() == rhs.getFrame() && stricmp(lhs.queryName(), rhs.queryName()) < 0)
        return true;
    return false;
}
