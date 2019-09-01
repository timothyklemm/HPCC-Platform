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
#ifndef ESDL_SCRIPT_CORE_IPP_
#define ESDL_SCRIPT_CORE_IPP_

#include "esdl_script_core.hpp"
#include "loggingmanager.h"
#include <list>

inline int compareCase(const char* lhs, const char* rhs)
{
    if (lhs == rhs)
        return false;
    if (nullptr == lhs)
        return false;
    if (nullptr == rhs)
        return true;
    return strcmp(lhs, rhs);
}

inline int compareNoCase(const char* lhs, const char* rhs)
{
    if (lhs == rhs)
        return false;
    if (nullptr == lhs)
        return false;
    if (nullptr == rhs)
        return true;
    return stricmp(lhs, rhs);
}

bool operator < (const EsdlScript::IStatement& lhs, const EsdlScript::IStatement& rhs)
{
    return compareNoCase(lhs.queryUID(), rhs.queryUID());
}

inline bool operator < (const String& lhs, const String& rhs)
{
    return compareNoCase(lhs.str(), rhs.str());
}

inline bool operator < (const EsdlScript::ILogAgentVariant& lhs, const EsdlScript::ILogAgentVariant& rhs)
{
    auto cmpResult = compareNoCase(lhs.getName(), rhs.getName());

    if (cmpResult != 0)
        return cmpResult < 0;
    cmpResult = compareNoCase(lhs.getType(), rhs.getType());
    if (cmpResult != 0)
        return cmpResult < 0;
    return compareNoCase(lhs.getGroup(), rhs.getGroup()) < 0;
}

template<typename TOwned>
bool operator < (const Owned<TOwned>& lhs, const Owned<TOwned>& rhs)
{
    auto lPtr = lhs.get();
    auto rPtr = rhs.get();

    if (lPtr == rPtr)
        return false;
    if (nullptr == lPtr)
        return false;
    if (nullptr == rPtr)
        return true;
    return *lPtr < *rPtr;
}

namespace EsdlScript
{
template <typename TEnum, TEnum undefined = TEnum(-1)>
struct TEnumMapper
{
    using Enum = TEnum;
    using MappingEntry = std::pair<Enum, const char*>;
    using Mapping = std::vector<MappingEntry>;

    Mapping m_mapping;

    TEnumMapper(const std::initializer_list<MappingEntry>& data)
        : m_mapping(data)
    {
    }

    Enum mapping(const char* label) const
    {
        if (isEmptyString(label))
            return undefined;
        auto it = std::find_if(m_mapping.begin(), m_mapping.end(), [label](const MappingEntry& element) { return strcmp(label, element.second) == 0; });
        return it != m_mapping.end() ? it->first : undefined;
    }

    const char* mapping(Enum e) const
    {
        auto it = std::find_if(m_mapping.begin(), m_mapping.end(), [e](const MappingEntry& element) { return element.first == e; });
        return it != m_mapping.end() ? it->second : nullptr;
    }
};

#define DECLARE_COWNER(token) \
struct C ## token ## Owner \
{ \
    using I ## token ## Creator = std::function<I ## token*()>; \
    C ## token ## Owner(I ## token ## Creator creator) : m_creator ## token(creator) {} \
    I ## token* query ## token() const { return m_contained ## token; } \
    I ## token* query ## token() { if (m_contained ## token.get() == nullptr) m_contained ## token.setown(m_creator ## token()); return m_contained ## token; } \
    I ## token* get ## token() const { return LINK(query ## token()); } \
    I ## token* get ## token() { return LINK(query ## token()); } \
private: \
    I ## token ## Creator m_creator ## token; \
    Owned<I ## token> m_contained ## token; \
}

#define IMPLEMENT_COWNER(token) \
    I ## token* query ## token() const { return C ## token ## Owner::query ## token(); } \
    I ## token* query ## token() { return C ## token ## Owner::query ## token(); } \
    I ## token* get ## token() const { return C ## token ## Owner::get ## token(); } \
    I ## token* get ## token() { return C ## token ## Owner::get ## token(); }


class CPersistPTree
{
public:
    IPTree* persist(IPTree* parent) const;
    void restore(const IPTree* node);
};

class COutcomeTypeContainer : implements IOutcomeTypeContainer
{
public:
    COutcomeTypeContainer(OutcomeType type) : m_type(type) {}
    ~COutcomeTypeContainer() {}

    OutcomeType getType() const override { return m_type; }

protected:
    const char* typeString() const
    {
        switch (m_type)
        {
        case OutcomeType_Success: return "success";
        case OutcomeType_Warning: return "warning";
        case OutcomeType_Error:   return "error";
        default:                  return "undefined";
        }
    }

protected:
    OutcomeType m_type;
};

class COutcomeContext : implements IOutcomeContext
{
public:
    COutcomeContext() {}
    ~COutcomeContext() {}

    const char* getComponent() const override { return (m_component.get() != nullptr ? m_component->str() : nullptr); }
    const char* getOperation() const override { return (m_operation.get() != nullptr ? m_operation->str() : nullptr); }
    void        setComponent(const char* component) { update(m_component, component); }
    void        setOperation(const char* operation) { update(m_operation, operation); }

protected:
    void update(Owned<String>& out, const char* in)
    {
        if (isEmptyString(in))
            out.clear();
        else if (out.get() == nullptr)
            out.setown(new String(in));
        else if (out->compareTo(in) != 0)
            out.setown(new String(in));
    }
protected:
    Owned<String> m_component;
    Owned<String> m_operation;
};

class COutcomes : implements IOutcomes, extends COutcomeTypeContainer, extends COutcomeContext, extends CPersistPTree, extends CInterface
{
protected:
    using History = std::list<Owned<IOutcome> >;
    class COutcome : implements IOutcome, extends COutcomeTypeContainer, extends COutcomeContext, extends CPersistPTree, extends CInterface
    {
    public:
        COutcome(String* component, String* operation, OutcomeType type, int code, const char* message);
        ~COutcome();

        IMPLEMENT_IINTERFACE;

        // IOutcomeTypeContainer
        OutcomeType getType() const override { return COutcomeTypeContainer::getType(); }

        // IOutcomeContext
        const char* getComponent() const override { return COutcomeContext::getComponent(); }
        const char* getOperation() const override { return COutcomeContext::getOperation(); }

        // IOutcome
        int getCode() const override { return m_code; }
        const char* getMessage() const override { return m_message.get(); }
        StringBuffer& toString(StringBuffer& buffer) const override;

        // IPersistent
        StringBuffer& persist(StringBuffer& xml) const override;
        IPTree* persist(IPTree* parent) const override { return CPersistPTree::persist(parent); }
        void restore(const IPTree* node) override { CPersistPTree::restore(node); }

    protected:
        int m_code;
        StringAttr m_message;
    };
    class COutcomeIterator : public CInterfaceOf<IOutcomeIterator>
    {
    public:
        COutcomeIterator(const COutcomes& outcomes);
        ~COutcomeIterator();

        // IVariableIterator
        virtual bool first() override;
        virtual bool next() override;
        virtual bool isValid() override;
        virtual const IOutcome & query() override;

    private:
        const COutcomes& m_outcomes;
        History::const_iterator m_outcomesIt;
    };

public:
    COutcomes() : COutcomeTypeContainer(OutcomeType_Undefined) {}

    IMPLEMENT_IINTERFACE;

    // IOutcomeTypeContainer
    OutcomeType getType() const override { return COutcomeTypeContainer::getType(); }

    // IOutcomeContext
    const char* getComponent() const override { return COutcomeContext::getComponent(); }
    const char* getOperation() const override { return COutcomeContext::getOperation(); }
    void setComponent(const char* component) override { COutcomeContext::setComponent(component); }
    void setOperation(const char* operation) override { COutcomeContext::setOperation(operation); }

    // IOutcomes
    void record(OutcomeType type, int code, const char* format, ...) override;
    void recordSuccess(int code = 0, const char* format = nullptr, ...) override;
    void recordWarning(int code, const char* format, ...) override;
    void recordError(int code, const char* format, ...) override;
    IOutcomeIterator* getOutcomes() const override;

    // IPersistent
    StringBuffer& persist(StringBuffer& xml) const override;
    IPTree* persist(IPTree* parent) const override { return CPersistPTree::persist(parent); }
    void restore(const IPTree* node) override { CPersistPTree::restore(node); }

protected:
    void record(IOutcome* outcome);
    virtual IOutcome* createOutcome(String* component, String* operation, OutcomeType type, int code, const char* message) const;

protected:
    History m_history;
};
DECLARE_COWNER(Outcomes);

struct OutcomesContext
{
    OutcomesContext(IOutcomes* outcomeHandler)
        : m_outcomeHandler(*outcomeHandler)
        , m_cachedComponent(m_outcomeHandler.getComponent())
        , m_cachedOperation(m_outcomeHandler.getOperation())
    {
    }

    OutcomesContext(IOutcomes& outcomeHandler)
        : m_outcomeHandler(outcomeHandler)
        , m_cachedComponent(m_outcomeHandler.getComponent())
        , m_cachedOperation(m_outcomeHandler.getOperation())
    {
    }

    OutcomesContext(IOutcomes& outcomeHandler, const char* operation = nullptr, const char* component = nullptr)
        : m_outcomeHandler(outcomeHandler)
        , m_cachedComponent(m_outcomeHandler.getComponent())
        , m_cachedOperation(m_outcomeHandler.getOperation())
    {
        if (!isEmptyString(component))
            m_outcomeHandler.setComponent(component);
        if (!isEmptyString(operation))
            m_outcomeHandler.setOperation(operation);
    }

    ~OutcomesContext()
    {
        m_outcomeHandler.setComponent(m_cachedComponent);
        m_outcomeHandler.setOperation(m_cachedOperation);
    }

    IOutcomes& operator * ()
    {
        return m_outcomeHandler;
    }

    bool isError() const
    {
        return m_outcomeHandler.isError();
    }

private:
    IOutcomes& m_outcomeHandler;
    StringBuffer     m_cachedComponent;
    StringBuffer     m_cachedOperation;
};


class CLoadContext : implements ILoadContext, extends CInterface, extends COutcomesOwner
{
public:
    CLoadContext()
        : COutcomesOwner([](){ return new COutcomes(); })
    {
    }

    IMPLEMENT_IINTERFACE;
    IMPLEMENT_COWNER(Outcomes);

    void addServiceConstraint(const char* service) override
    {
        if (!isEmptyString(service))
            m_serviceConstraints.insert(service);
    }
    void removeServiceConstraint(const char* service) override
    {
        if (!isEmptyString(service))
            m_serviceConstraints.erase(service);
    }
    void clearServiceConstraints() override
    {
        m_serviceConstraints.clear();
    }
    bool isServiceConstraint(const char* service) const override
    {
        if (isEmptyString(service))
            return false;
        else if (m_serviceConstraints.empty())
            return true;
        else
            return m_serviceConstraints.count(service) > 0;
    }

    void addPhaseExclusion(Phase phase) override
    {
        m_phaseExclusions.insert(phase);
    }
    void removePhaseExclusion(Phase phase) override
    {
        m_phaseExclusions.erase(phase);
    }
    void clearPhaseExclusions() override
    {
        m_phaseExclusions.clear();
    }
    bool isPhaseExclusion(Phase phase) const override
    {
        return m_phaseExclusions.count(phase) > 0;
    }

    void setInput(const StringBuffer& xml) override
    {
        if (xml.length() > LONG_MAX)
            throw MakeStringException(-1, "XmlPullParser context initialization error [XML content too big (%u)]", xml.length());
        m_parser->setInput(xml, int(xml.length()));
    }
    bool next(int& entityType) override
    {
        m_content.clear();
        entityType = m_entityType = m_parser->next();
        switch (entityType)
        {
        case XmlPullParser::END_DOCUMENT:
            return false;
        case XmlPullParser::START_TAG:
            m_parser->readStartTag(m_startTag);
            m_tagName = m_startTag.getLocalName();
            break;
        case XmlPullParser::CONTENT:
            m_content = m_parser->readContent();
            break;
        case XmlPullParser::END_TAG:
            m_parser->readEndTag(m_endTag);
            break;
        default:
            m_tagName = nullptr;
            break;
        }
        return true;
    }
    bool next() override
    {
        int entityType;
        return next(entityType);
    }
    void skip() override
    {
        m_parser->skipSubTree();
    }
    int currentEntityType() const override
    {
        return m_entityType;
    }
    const char* currentTag() const override
    {
        return m_tagName;
    }
    const char* currentAttribute(const char* name) const override
    {
        if (m_tagName != nullptr)
            return m_startTag.getValue(name);
        return nullptr;
    }
    const char* currentContent() const override
    {
        return m_content.c_str();
    }
    bool isCurrentContentSpace() const override
    {
        return m_content.find_first_not_of(" \t\r\n") == std::string::npos;
    }

    void setCurrentService(const char* service) override
    {
        m_currentService.set(service);
    }
    const char* currentService() const override
    {
        return m_currentService;
    }
    void setCurrentMethod(const char* method) override
    {
        m_currentMethod.set(method);
    }
    const char* currentMethod() const override
    {
        return m_currentMethod;
    }

    const IFactory* queryFactory() const override
    {
        return m_factory;
    }

protected:
    using ServiceConstraints = std::set<std::string>;
    using PhaseExclusions = std::set<Phase>;

    ServiceConstraints             m_serviceConstraints;
    PhaseExclusions                m_phaseExclusions;
    std::unique_ptr<XmlPullParser> m_parser;
    int                            m_entityType = XmlPullParser::END_DOCUMENT;
    StartTag                       m_startTag;
    std::string                    m_content;
    EndTag                         m_endTag;
    const char*                    m_tagName = nullptr;
    StringBuffer                   m_currentService;
    StringBuffer                   m_currentMethod;
    Linked<const IFactory>         m_factory;

    friend class CEnvironment;
};

class CTraceState : implements ITraceState, extends CInterface, extends CPersistPTree
{
public:
    CTraceState();
    ~CTraceState();

    IMPLEMENT_IINTERFACE;

    // IPersistent
    StringBuffer& persist(StringBuffer& xml) const override;
    IPTree* persist(IPTree* parent) const override { return CPersistPTree::persist(parent); }
    void restore(const IPTree* node) override;

    // ITraceState
    void pushTraceStateFrame() override;
    void setLogLevel(TraceType type, LogLevel level, TraceFrame inFrame = TraceFrame_Current) override;
    LogLevel getLogLevel(TraceType type) const override;
    void popTraceStateFrame() override;
    StringBuffer& toString(StringBuffer& buffer) const override;

public:
    using TSFrame = std::map<TraceType, LogLevel>;
protected:
    using TSStack = std::vector<TSFrame>;
    TSStack m_tsStack;
};

struct StTraceStateFrame
{
    StTraceStateFrame(ITraceState& stack)
        : _stack(stack)
    {
        _stack.pushTraceStateFrame();
        _pushed = true;
    }
    ~StTraceStateFrame()
    {
        if (_pushed)
            _stack.popTraceStateFrame();
    }

private:
    ITraceState& _stack;
    bool         _pushed = false;
};

class CLibrary : implements ILibrary, extends CInterface
{
public:
    CLibrary();
    ~CLibrary();

    IMPLEMENT_IINTERFACE;

    OutcomeType load(ILoadContext& context) override;
    const IStatement* queryStatement(const char* uid) const override;
    const IStatement* getStatement(const char* uid) const override;

protected:
    using Statements = std::map<std::string, Owned<IStatement> >;
    Statements    m_statements;
    mutable ReadWriteLock m_statementsLock;
};
DECLARE_COWNER(Library);

class CFactory : implements IFactory, extends CInterface
{
public:
    CFactory();
    ~CFactory();

    IMPLEMENT_IINTERFACE;

    void initialize() override;

    OutcomeType registerStatement(const char* tag, Creator creator, IOutcomes* outcomes = nullptr) override;
    bool isStatement(const char* name) const override;
    IStatement* create(ILoadContext& context, IStatement* parent = nullptr) const override;
    OutcomeType unregisterStatement(const char* tag, IOutcomes* outcomes = nullptr) override;

protected:
    using CreatorMap = std::map<std::string, Creator>;
    CreatorMap m_creatorMap;
    mutable ReadWriteLock m_creatorLock;
};
DECLARE_COWNER(Factory);

class CStatement : implements IStatement, extends CInterface
{
public:
    enum InitializationState
    {
        InitializationState_New,
        InitializationState_InProgress,
        InitializationState_Complete,
    };

    static const uint8_t ChildPredicate_NoMatch = 0;
    static const uint8_t ChildPredicate_Match = 1;
    static const uint8_t ChildPredicate_Continue = 0;
    static const uint8_t ChildPredicate_Stop = 2;

    using ParentAcceptor = std::function<bool(const CStatement*, const IStatement*)>;
    using ChildAcceptor = std::function<bool(const CStatement*, const char*)>;
    using ExtensionAcceptor = std::function<bool(const CStatement*, const char*)>;
    using ChildPredicate = std::function<uint8_t(const Owned<IStatement>&, IProcessContext*, IParentContext*)>;
    using Children = std::list<Owned<IStatement> >;

    CStatement();
    ~CStatement();

    IMPLEMENT_IINTERFACE;

    IStatement* queryChildTag(const char* tag) const;
    IStatement* queryChildUID(const char* uid) const;

    // class configuration
protected:
    void setParentAcceptor(ParentAcceptor acceptor);
    void setChildAcceptor(ChildAcceptor acceptor);
    void setExtensionAcceptor(ExtensionAcceptor acceptor);
    void setChildPredicate(ChildPredicate predicate);

    // instance initialization
public:
    bool initialize(ILoadContext& context) override;
protected:
    virtual bool initializeSelf(ILoadContext& context);
    virtual void handleStartTag(ILoadContext& context);
    virtual void handleContent(ILoadContext& context);
    virtual void handleEndTag(ILoadContext& context);
    virtual void extendSelf(ILoadContext& context);
    virtual void acceptChild(IStatement* child);
    virtual void validateSelf(ILoadContext& context);
public:
    bool acceptsChildren() const override;
    bool acceptsContent() const override;
    bool acceptsExtensions() const override;
    bool acceptsParent(const IStatement* parent) const override;
    bool acceptsChild(const char* name) const override;
    bool acceptsExtension(const char* name) const override;
    void checkPhase(ILoadContext& context) const override;
protected:
    bool isNew() const { return InitializationState_New == m_initializationState; }
    bool isInitializing() const { return InitializationState_InProgress == m_initializationState; }
    bool isInitialized() const { return InitializationState_Complete == m_initializationState; }

    void setAcceptsChildren(bool accepts = true);
    void setAcceptsContent(bool accepts = false);
    void setAcceptsExtensions(bool accepts = false);
    bool isChild(const IStatement* statement) const;
    Children::iterator find(const IStatement* child);
    Children::const_iterator find(const IStatement* child) const;

    // transaction processing
public:
    const char* queryReadCursor() const;
    const char* queryWriteCursor() const;
    void process(IProcessContext& context, IParentContext*  parentInfo, const char* readXPath, const char* writeXPath) const override;
protected:
    bool processSelf(IProcessContext& context, IParentContext* parentInfo, const char* readXPath, const char* writeXPath, Owned<IParentContext>& selfInfo) const;
    void processChildren(IProcessContext& context, IParentContext* parentInfo, const char* readXPath, const char* writeXPath) const;

    // conditional evaluation
public:
    bool isEvaluable() const override;
    bool evaluate(IProcessContext* context, IParentContext* parentInfo) const override;
    bool isComparable() const override;
    bool compare(const char* value, IProcessContext* context) const override;

    // instance information
public:
    const char* queryTag() const override;
    const char* queryUID() const override;
    Phase queryPhase() const override;

    // instance data
protected:
    Children m_children;
private:
    ParentAcceptor m_parentAcceptor;
    ChildAcceptor m_childAcceptor;
    ExtensionAcceptor m_extensionAcceptor;
    ChildPredicate m_childPredicate;
    StringBuffer m_tag;
    StringBuffer m_uid;
    IStatement* m_parent = nullptr;
    InitializationState m_initializationState = InitializationState_New;
    bool m_acceptsChildren = true;
    bool m_acceptsContent = false;
    bool m_acceptsExtensions = false;
};

class CEnvironment : implements IEnvironment, extends CInterface, extends CLibraryOwner, extends CFactoryOwner
{
public:
    CEnvironment();
    ~CEnvironment();

    IMPLEMENT_IINTERFACE;
    IMPLEMENT_COWNER(Factory);
    IMPLEMENT_COWNER(Library);

    ILoadContext* createLoadContext() const override;

    OutcomeType load(const IPTree* binding, const char* service = nullptr) override;
    OutcomeType load(const Script& script, const char* service = nullptr) override;
    void load(ILoadContext& context, const IPTree* binding) override;
    void load(ILoadContext& context, const Script& script) override;

    IProcessContext* createProcessContext(IEspContext& espContext, const char* service, const char* method) const override;
    IProcessContext* restoreProcessContext(const char* logXML) const override;

    PhaseResult processPhase(IProcessContext& context, Phase phase) const override;

protected:
    virtual void load(ILoadContext& context, const ScriptFragment& fragment);
    virtual void loadScript(ILoadContext& context);
    virtual void loadLibrary(ILoadContext& context);
    virtual void loadPhase(ILoadContext& context, Phase phase);
    virtual void checkSyntax(ILoadContext& context) const;

protected:
    template <class TMap>
    struct SynchronizableMap
    {
        TMap first;
        mutable ReadWriteLock second;
    };
    using PhaseMap = std::map<Phase, Owned<IStatement> >;
    using MethodMap = std::map<std::string, SynchronizableMap<PhaseMap> >;
    using ServiceMap = std::map<std::string, SynchronizableMap<MethodMap> >;
    ServiceMap      m_serviceMap;
    mutable ReadWriteLock   m_serviceMapLock;
    Owned<ILibrary> m_library;
    Owned<IFactory> m_factory;

protected:
    virtual void loadPhase(ILoadContext& context, Owned<IStatement>& statement, Phase phase);
    virtual void checkPhase(ILoadContext& context, const Owned<IStatement>& statement) const;
};

struct Abort {};
struct Fail  {};

class CVariables : implements IVariables, extends CPersistPTree, extends CInterface
{
protected:
    interface IMutableVariable : extends IVariable
    {
        virtual void updateState(VariableState state) = 0;
        virtual void updateFrame(VariableFrame frame) = 0;
        virtual void updateValue(const char* value) = 0;
    };
    using VariableSet = std::set<Owned<IMutableVariable> >;
    using VariableSetStack = std::vector<VariableSet>;
    class CVariable : implements IMutableVariable, extends CPersistPTree, extends CInterface
    {
    public:
        CVariable(VariableState state, const char* name, const char* value, VariableFrame frame);
        ~CVariable();

        IMPLEMENT_IINTERFACE;

        // IMutableVariable
        void updateState(VariableState state) override { m_state = state; }
        void updateFrame(VariableFrame frame) override { m_frame = frame; }
        void updateValue(const char* value) override { m_value.set(value); }

        // IVariable
        VariableState getState() const override { return m_state; }
        VariableFrame getFrame() const override { return m_frame; }
        const char* queryName() const override { return m_name; }
        const char* queryValue() const override { return m_value; }

        // IPersistent
        StringBuffer& persist(StringBuffer& xml) const override;
        IPTree* persist(IPTree* parent) const override { return CPersistPTree::persist(parent); }
        void restore(const IPTree* node) override { CPersistPTree::restore(node); }

    protected:
        VariableState      m_state;
        VariableFrame m_frame;
        StringBuffer       m_name;
        StringBuffer       m_value;
    };
    class CVariableIterator : public CInterfaceOf<IVariableIterator>
    {
    public:
        CVariableIterator(const CVariables& stack, VariableFrame maxFrame, VariableFrame minFrame);
        ~CVariableIterator();

        // IVariableIterator
        virtual bool first() override;
        virtual bool next() override;
        virtual bool isValid() override;
        virtual const IVariable & query() override;

    protected:
        inline VariableFrame frameCount() const { return VariableFrame(m_stack.m_data.size()); }

    private:
        const CVariables&           m_stack;
        VariableFrame          m_maxFrame;
        VariableFrame          m_minFrame;
        VariableFrame          m_curFrame = CURRENT_VARIABLE_FRAME;
        VariableSet::const_iterator m_varIt;
    };
public:
    CVariables();
    ~CVariables();

    IMPLEMENT_IINTERFACE;

    StringBuffer& persist(StringBuffer& xml) const override;
    IPTree* persist(IPTree* parent) const override { return CPersistPTree::persist(parent); }
    void restore(const IPTree* node) override;

    IVariableListener* getVariableListener() const override;
    void setVariableListener(IVariableListener* listener) override;
    VariableFrame pushVariablesFrame() override;
    OutcomeType addVariable(VariableState state, const char* name, const char* value, VariableFrame atFrame = CURRENT_VARIABLE_FRAME, IOutcomes* outcomes = nullptr) override;
    bool isVariable(const char* name, VariableFrame maxFrame = CURRENT_VARIABLE_FRAME, VariableFrame minFrame = 0) const override;
    const char* queryVariable(const char* name, VariableFrame maxFrame = CURRENT_VARIABLE_FRAME, VariableFrame minFrame = 0) const override;
    void popVariablesFrame() override;
    IVariableIterator* getVariables(VariableFrame maxFrame = CURRENT_VARIABLE_FRAME, VariableFrame minFrame = 0) const override;

protected:
    virtual void applyVariableUpdate(const char* name, const char* value);

protected:
    virtual IMutableVariable* createVariable(VariableState state, const char* name, const char* value, VariableFrame frame = CURRENT_VARIABLE_FRAME) const;
    inline  IMutableVariable* createDeclaredVariable(const char* name, const char* value, VariableFrame frame = CURRENT_VARIABLE_FRAME) const { return createVariable(VariableState_Declared, name, value, frame); }
    inline  IMutableVariable* createDefinedVariable(const char* name, const char* value, VariableFrame frame = CURRENT_VARIABLE_FRAME) const { return createVariable(VariableState_Defined, name, value, frame); }

protected:
    IVariableListener* m_listener = nullptr;
    VariableSetStack m_data;
};

class CLogAgentFilter : implements ILogAgentFilter, extends CInterface
{
protected:
    using Filters = std::list<Owned<ILogAgentFilter> >;
    using Variants = std::set<const ILogAgentVariant*>;
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
    bool               includes(const ILogAgentVariant* agent) const override;

protected:
    virtual bool isMatch(const char* value, const char* pattern) const;
    virtual bool doMatch(bool matched, const ILogAgentVariant* variant, LogAgentFilterMode mode);

protected:
    LogAgentFilterMode m_mode;
    LogAgentFilterType m_type;
    StringAttr         m_pattern;
    Filters            m_refinements;
    Variants           m_variants;
};

class CLogAgentState : implements ILogAgentState, extends CLogAgentFilter, extends CPersistPTree
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
    bool               includes(const ILogAgentVariant* variant) const override { return CLogAgentFilter::includes(variant); }

    // IPersistent
    StringBuffer& persist(StringBuffer& xml) const override;
    IPTree* persist(IPTree* parent) const override { return CPersistPTree::persist(parent); }
    void restore(const IPTree* node) override;

protected:
    using Strings = std::set<Owned<String> >;
    static Variants transform(const ILoggingManager* manager);
};

// -------------------------------------------------------------------------------------------------

class CPhaseStatement : extends CStatement
{
public:
    CPhaseStatement(Phase phase);
    ~CPhaseStatement();

    Phase queryPhase() const override;

protected:
    void checkPhase(ILoadContext& context) const override;

private:
    Phase m_phase = Phase_Unknown;
};

class CCustomRequestTransformStatement : extends CStatement
{

};

class CChooseStatement : extends CStatement
{

};

class CWhenStatement : extends CStatement
{

};

class CSetValueStatement : extends CStatement
{

};

class CAppendValueStatement : extends CStatement
{

};

class CSkipLogAgentStatement : extends CStatement
{

};

class CTraceStateStatement : extends CStatement
{

};

class CDebugStatement : extends CStatement
{

};

} // namespace EsdlScript

extern bool operator < (const EsdlScript::IVariable& lhs, const EsdlScript::IVariable& rhs);
extern bool operator < (const Owned<EsdlScript::IVariable>& lhs, const Owned<EsdlScript::IVariable>& rhs);

#endif // ESDL_SCRIPT_CORE_IPP_
