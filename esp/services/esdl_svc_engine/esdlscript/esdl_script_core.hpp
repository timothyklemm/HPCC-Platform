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
#ifndef ESDL_SCRIPT_CORE_HPP_
#define ESDL_SCRIPT_CORE_HPP_

#include "esp.hpp"
#include "jliball.hpp"
#include "loggingagentbase.hpp"
#include "xpathprocessor.hpp"

#include <functional>
#include <initializer_list>
#include <list>
#include <memory>
#include <set>

namespace EsdlScript
{
interface IProcessContext;
interface ILibrary;
interface IStatement;
interface IFactory;

namespace Tags
{
    extern const char* root;
    extern const char* library;
    extern const char* phasePreflight;
    extern const char* phaseRequest;
    extern const char* phaseResponse;
    extern const char* phaseLogManager;
    extern const char* phaseLogAgent;
}

enum Phase
{
    Phase_Unknown = -1,
    Phase_Preflight,
    Phase_Request,
    Phase_Response,
    Phase_LogManager,
    Phase_LogAgent,
};

// Specifies a standard serialization interface to be used by derived classes to save their content
// into either a text or tree representation of XML.
interface IPersistent
{
    virtual StringBuffer& persist(StringBuffer& xml) const = 0;
    virtual IPTree* persist(IPTree* parent) const = 0;
    virtual void restore(const IPTree* node) = 0;
};

// High-level outcome of an ESDL request, such as statement processing. Callers can use this
// value to determine how to proceed without making assumptions about specific status codes.
enum OutcomeType
{
    OutcomeType_Undefined = -1,
    OutcomeType_Success, // processing completed with no unexpected outcome
    OutcomeType_Warning, // processing completed with non-fatal errors
    OutcomeType_Error,   // unable to complete processing
};

interface IOutcomeTypeContainer
{
    virtual OutcomeType getType() const = 0;
    inline operator OutcomeType() const { return getType(); }
    inline bool isSuccess() const { return getType() == OutcomeType_Success; }
    inline bool isWarning() const { return getType() == OutcomeType_Warning; }
    inline bool isError()   const { return getType() == OutcomeType_Error; }
};

interface IOutcomeContext
{
    virtual const char* getComponent() const = 0;
    virtual const char* getOperation() const = 0;
};

interface IOutcome : extends IInterface, extends IOutcomeTypeContainer, extends IOutcomeContext, extends IPersistent
{
    virtual int getCode() const = 0;
    virtual const char* getMessage() const = 0;

    virtual StringBuffer& toString(StringBuffer& buffer) const = 0;
};

interface IOutcomeIterator : extends IIteratorOf<const IOutcome> {};

interface IOutcomes : extends IInterface, extends IOutcomeTypeContainer, extends IOutcomeContext, extends IPersistent
{
    virtual void setComponent(const char* component) = 0;
    virtual void setOperation(const char* operation) = 0;
    virtual void record(OutcomeType type, int code, const char* format, ...) = 0;
    virtual void recordSuccess(int code = 0, const char* format = nullptr, ...) = 0;
    virtual void recordWarning(int code, const char* format, ...) = 0;
    virtual void recordError(int code, const char* format, ...) = 0;
    virtual IOutcomeIterator* getOutcomes() const = 0;
};

#define DECLARE_IOWNER(token) \
interface I ## token ## Owner \
{ \
    virtual I ## token* query ## token() const = 0; \
    virtual I ## token* query ## token() = 0; \
    virtual I ## token* get ## token() const = 0; \
    virtual I ## token* get ## token() = 0; \
}

DECLARE_IOWNER(Outcomes);

struct ILoadContext : extends IInterface, implements IOutcomesOwner
{
    virtual void addServiceConstraint(const char* service) = 0;
    virtual void removeServiceConstraint(const char* service) = 0;
    virtual void clearServiceConstraints() = 0;
    virtual bool isServiceConstraint(const char* service) const = 0;

    virtual void addPhaseExclusion(Phase phase) = 0;
    virtual void removePhaseExclusion(Phase phase) = 0;
    virtual void clearPhaseExclusions() = 0;
    virtual bool isPhaseExclusion(Phase phase) const = 0;

    virtual void setInput(const StringBuffer& xml) = 0;
    virtual bool next(int& entityType) = 0;
    virtual bool next() = 0;
    virtual void skip() = 0;
    virtual int  currentEntityType() const = 0;
    inline  bool atStartTag() const { return currentEntityType() == XmlPullParser::START_TAG; }
    inline  bool atContent() const  { return currentEntityType() == XmlPullParser::CONTENT; }
    inline  bool atEndTag() const   { return currentEntityType() == XmlPullParser::END_TAG; }
    virtual const char* currentTag() const = 0;
    virtual const char* currentAttribute(const char* name) const = 0;
    virtual const char* currentContent() const = 0;
    virtual bool isCurrentContentSpace() const = 0;

    virtual void setCurrentService(const char* service) = 0;
    virtual const char* currentService() const = 0;
    virtual void setCurrentMethod(const char* method) = 0;
    virtual const char* currentMethod() const = 0;

    virtual const IFactory* queryFactory() const = 0;
};

// The source of all script statements. An implementation may include support for some set of tags,
// and should support external registration of new tags. An external registration may overwrite any
// built in type.
//
// Instantiation of a statement must instantiate all statements defined within it. While most
// statements are defined by a single element, some definitions may span multiple elements (e.g.,
// an Option child of a MySQLServer parent statement). The factory does not know how a statement
// is defined, so the statement must drive instantiation of its children.
interface IFactory : extends IInterface
{
    using Creator = std::function<IStatement*()>;

    virtual void initialize() = 0;

    // Install a creation function for statements with the given tag. Returns success for a new
    // tag, warning if the type is already registered, and error if inputs are invalid.
    virtual OutcomeType registerStatement(const char* tag, Creator creator, IOutcomes* outcomes = nullptr) = 0;
    //
    virtual bool isStatement(const char* name) const = 0;
    //
    virtual IStatement* create(ILoadContext& context, IStatement* parent = nullptr) const = 0;
    // Uninstall the creation function for the given tag. Returns success if the tag is no longer
    // registered, warning if tag reverts to a previous registration, or error if the inputs are
    // invalid.
    virtual OutcomeType unregisterStatement(const char* tag, IOutcomes* outcomes = nullptr) = 0;
};
DECLARE_IOWNER(Factory);

// The repository of all uniquely identified script statements in a binding. Lookups are reentrant.
interface ILibrary : extends IInterface
{
    virtual OutcomeType load(ILoadContext& context) = 0;
    virtual const IStatement* queryStatement(const char* uid) const = 0;
    virtual const IStatement* getStatement(const char* uid) const = 0;
};
DECLARE_IOWNER(Library);

interface IParentContext : extends IInterface
{
};

// An executable script construct.
interface IStatement : extends IInterface
{
    virtual bool isInitialized() const = 0;
    virtual bool acceptsChildren() const = 0;
    virtual bool acceptsContent() const = 0;
    virtual bool acceptsExtensions() const = 0;
    virtual bool acceptsParent(const IStatement* parent) const = 0;
    virtual bool acceptsChild(const char* name) const = 0;
    virtual bool acceptsExtension(const char* name) const = 0;
    virtual void checkPhase(ILoadContext& context) const = 0;

    // Initializes the current statement and all descendants using data from the pull parser.
    // On entry, the parser is at the start tag of the current statement.
    // On exit, the parser must be at the end tag of the current statement, where parser.next()
    // will advance to the next sibling (or an ancestor). If the result is OutcomeType_Error then
    // the parser is not required to advance to the end tag.
    virtual bool initialize(ILoadContext& context) = 0;

    virtual const char* queryTag() const = 0;
    virtual const char* queryUID() const = 0;
    virtual Phase queryPhase() const = 0;

    // Called during transaction processing, executes  subclass-specific behaviors.
    virtual const char* queryReadCursor() const = 0;
    virtual const char* queryWriteCursor() const = 0;
    virtual void process(IProcessContext& context, IParentContext* parentInfo, const char* readXPath, const char* writeXPath) const = 0;

    // Returns true if and only if the expected outcome of the statement is the production of a
    // Boolean value. Most statements will return false, while logical control statements will
    // return true.
    virtual bool isEvaluable() const = 0;
    // Returns the Boolean value produced by an evaluable statement, such as 'if' or 'when'.
    // Returns false always when the statement is not evaluable.
    virtual bool evaluate(IProcessContext* context, IParentContext* parentInfo) const = 0;

    virtual bool isComparable() const = 0;
    virtual bool compare(const char* value, IProcessContext* context) const = 0;
};

enum TraceType
{
    TraceType_Undefined = -1,

    // corresponding to OutcomeType values
    TraceType_Success,
    TraceType_Warning,
    TraceType_Error,

    // explicit logging request types
    TraceType_CallStack,

    // user-defined values
    TraceType_UserInfo1,
    TraceType_UserInfo2,
    TraceType_UserInfo3,
};
extern const char* mapTraceType(TraceType type);
extern TraceType   mapTraceType(const char* label);

enum TraceFrame
{
    TraceFrame_Current = -1,
    TraceFrame_Script,
    TraceFrame_Phase,
};

interface ITraceState : extends IInterface, extends IPersistent
{
    struct StFrame
    {
        StFrame(ITraceState& stack) : m_stack(stack) { m_stack.pushTraceStateFrame(); }
        ~StFrame() { m_stack.popTraceStateFrame(); }

        ITraceState& m_stack;
    };

    virtual void pushTraceStateFrame() = 0;
    virtual void setLogLevel(TraceType type, LogLevel level, TraceFrame inFrame = TraceFrame_Current) = 0;
    virtual LogLevel getLogLevel(TraceType type) const = 0;
    virtual void popTraceStateFrame() = 0;

    virtual StringBuffer& toString(StringBuffer& buffer) const = 0;
};

DECLARE_IOWNER(TraceState);

interface IReadCursorStack : extends virtual IInterface, extends virtual IPersistent
{
    virtual void        pushReadCursor(const char* xpath) = 0;
    virtual const char* evaluate(const char* xpath, StringBuffer& result) const = 0;
    virtual const char* evaluate(ICompiledXpath* xpath, StringBuffer& result) const = 0;
    virtual bool        evaluate(const char* xpath) const = 0;
    virtual bool        evaluate(ICompiledXpath* xpath) const = 0;
    virtual void        popReadCursor() = 0;
};

interface IWriteCursorStack : extends virtual IInterface, extends virtual IPersistent
{
    virtual void    pushWriteCursor(const char* xpath) = 0;
    virtual IPTree* queryBranch(bool ensureExistence = true) const = 0;
    virtual void    popWriteCursor() = 0;
};

enum VariableState
{
    VariableState_Undefined = -1, // variable instance is uninitialized
    VariableState_Declared,       // variable name has been declared and a default value assigned
    VariableState_Defined,        // variable name has been defined with a runtime value
};
extern const char* mapVariableState(VariableState state);
extern VariableState mapVariableState(const char* label);

using VariableFrame = uint32_t;
static const auto CURRENT_VARIABLE_FRAME = VariableFrame(-1);
static const VariableFrame GLOBAL_VARIABLE_FRAME = 0;
static const VariableFrame SCRIPT_VARIABLE_FRAME = 1;
static const VariableFrame PHASE_VARIABLE_FRAME  = 2;

interface IVariable : extends IInterface, extends IPersistent
{
    virtual VariableState getState() const = 0;
    inline bool isDeclared() const { return getState() == VariableState_Declared; }
    inline bool isDefined() const { return getState() == VariableState_Defined; }

    virtual VariableFrame getFrame() const = 0;
    inline bool isGlobalScope() const { return getFrame() == GLOBAL_VARIABLE_FRAME; }
    inline bool isScriptScope() const { return getFrame() == SCRIPT_VARIABLE_FRAME; }
    inline bool isPhaseScope() const { return getFrame() == PHASE_VARIABLE_FRAME; }

    virtual const char* queryName() const = 0;
    virtual const char* queryValue() const = 0;
};

interface IVariableIterator : extends IIteratorOf<const IVariable> {};

interface IVariableListener
{
    virtual void applyVariableUpdate(const char* name, const char* value) = 0;
};

// Variables are maintained in a stack, where a variable defined in a stack frame is visible in all
// following frames but is not visible to an preceding stack frame. This interface deviates from
// the traditional stack pattern by allowing direct access to any stack frame, enabling variable
// visibility to be controlled regardless of when a variable can be defined.
interface IVariables : extends virtual IInterface, extends virtual IPersistent
{
    struct StFrame
    {
        StFrame(IVariables& stack) : m_stack(stack) { m_stack.pushVariablesFrame(); }
        ~StFrame() { m_stack.popVariablesFrame(); }

        IVariables& m_stack;
    };

    virtual IVariableListener* getVariableListener() const = 0;
    virtual void setVariableListener(IVariableListener* listener) = 0;

    // Create a new variable stack frame based on the current frame.
    // Return the stack depth of the new set
    virtual VariableFrame pushVariablesFrame() = 0;
    // Adds a new variable visible to stack frame 'atFrame' and all newer frames
    virtual OutcomeType addVariable(VariableState state, const char* name, const char* value, VariableFrame atFrame = CURRENT_VARIABLE_FRAME, IOutcomes* outcomes = nullptr) = 0;
    inline OutcomeType declareVariable(const char* name, const char* value, VariableFrame atFrame = CURRENT_VARIABLE_FRAME, IOutcomes* outcomes = nullptr) { return addVariable(VariableState_Declared, name, value, atFrame, outcomes); }
    inline OutcomeType defineVariable(const char* name, const char* value, VariableFrame atFrame = CURRENT_VARIABLE_FRAME, IOutcomes* outcomes = nullptr) { return addVariable(VariableState_Defined, name, value, atFrame, outcomes); }

    // Returns true only if the variable name is defined in one of the indicated frames
    virtual bool isVariable(const char* name, VariableFrame maxFrame = CURRENT_VARIABLE_FRAME, VariableFrame minFrame = 0) const = 0;
    // Returns the value of the variable defined in on of the indicated frames
    virtual const char* queryVariable(const char* name, VariableFrame maxFrame = CURRENT_VARIABLE_FRAME, VariableFrame minFrame = 0) const = 0;
    // Removes the current variable stack frame
    virtual void popVariablesFrame();
    // Returns a collection of all variables defined in the indicated range of frames.
    virtual IVariableIterator* getVariables(VariableFrame maxFrame = CURRENT_VARIABLE_FRAME, VariableFrame minFrame = GLOBAL_VARIABLE_FRAME) const = 0;
};

DECLARE_IOWNER(Variables);

// -------------------------------------------------------------------------------------------------

// -------------------------------------------------------------------------------------------------

interface ICursor : extends IInterface, extends IVariableListener
{
    virtual bool setRoot(const char* xpath) = 0;
    virtual const char* queryRoot() const = 0;

protected:
    struct StFrameBase
    {
        StFrameBase(ICursor& cursor, const char* newRoot)
            : m_cursor(cursor)
        {
            if (!isEmptyString(newRoot))
            {
                m_currentRoot.append(m_cursor.queryRoot());
                m_valid = m_cursor.setRoot(newRoot);
                m_updated = true;
            }
        }
        virtual ~StFrameBase()
        {
            if (m_updated)
            {
                m_cursor.setRoot(m_currentRoot);
            }
        }

        bool isValid() const
        {
            return m_valid;
        }

    protected:
        ICursor&     m_cursor;
        StringBuffer m_currentRoot;
        bool         m_updated = false;
        bool         m_valid = true;
    };
};

interface IReadCursor : extends ICursor
{
    struct StFrame : extends StFrameBase
    {
        StFrame(IReadCursor& cursor, const IStatement& statement) : StFrameBase(cursor, statement.queryReadCursor()) {}
        ~StFrame() {}
    };

    virtual const char* evaluate(const char* xpath, StringBuffer& result) const = 0;
    virtual const char* evaluate(ICompiledXpath* xpath, StringBuffer& result) const = 0;
    virtual bool        evaluate(const char* xpath) const = 0;
    virtual bool        evaluate(ICompiledXpath* xpath) const = 0;
    // TODO: evaluate to single node, multiple nodes; need to restore original state when finished
};
DECLARE_IOWNER(ReadCursor);

interface IWriteCursor : extends ICursor
{
    struct StFrame : extends StFrameBase
    {
        StFrame(IWriteCursor& cursor, const IStatement& statement) : StFrameBase(cursor, statement.queryWriteCursor()) {}
        ~StFrame() {}
    };

    virtual IPTree* resolveRoot() const = 0;
    virtual IPTree* queryBranch(const char* xpath, bool ensure = true) const = 0;
};
DECLARE_IOWNER(WriteCursor);

interface IProcessContext : extends virtual IInterface,
                            extends virtual IPersistent,
                            extends IReadCursorOwner,
                            extends IWriteCursorOwner,
                            extends ITraceStateOwner,
                            extends IVariablesOwner,
                            extends IVariableListener,
                            extends IOutcomesOwner,
                            extends ILogAgentStateOwner,
                            extends ILibraryOwner
{
    virtual IEspContext* queryEspContext() const = 0;
    virtual const char* queryCurrentService() const = 0;
    virtual const char* queryCurrentMethod() const = 0;
    virtual Phase queryCurrentPhase() const = 0;

    virtual const IStatement* queryCurrentStatement() const = 0;
};

struct ScriptFragment
{
    StringBuffer content;
    const char*  service = nullptr;
    const char*  method = nullptr;
};
class Script : extends std::list<ScriptFragment>
{
public:
    using Base = std::list<ScriptFragment>;
    using Base::Base;

    bool add(const IPTree* tree, const char* xpath, const char* service, const char* context = nullptr);
};

enum PhaseResult
{
    PhaseResult_Unknown = -1,
    PhaseResult_Success,             // all statements completed without error
    PhaseResult_Aborted,             // statement processing terminated without error
    PhaseResult_Failed,              // statement processing terminated with error
    PhaseResult_AbnormalTermination, // statement processing exception
};

/**
 * A run-time representation of one or more EsdlScript definitions.
 */
interface IEnvironment : extends IInterface,
                         extends ILibraryOwner,
                         extends IFactoryOwner
{
    // Creates a loading context to be used for one or more calls to 'load'. The omission of context
    // when calling load will cause a default context to be used.
    virtual ILoadContext* createLoadContext() const = 0;


    // Loads one or all service definitions within the binding property tree, depending upon the
    // value of 'service'; all definition are used when omitted or empty, and only the named service
    // is used when specified.
    //
    // The instance will be initialized using all data deemed relevant in the property tree. Legacy
    // script content (e.g., 'Methods/xsdl:CustomRequestContent') is handled.
    virtual OutcomeType load(const IPTree* binding, const char* service = nullptr) = 0;
    virtual void load(ILoadContext& context, const IPTree* binding) = 0;

    // Loads one or all service definitions within the collection of script fragments, depending
    // upon the value of 'service'.
    //
    // The instance will be initialized using only the data given. It is the caller's responsibility
    // to convert legacy content into suitable ScriptFragments.
    virtual OutcomeType load(const Script& script, const char* service = nullptr) = 0;
    virtual void load(ILoadContext& context, const Script& script) = 0;


    // Constructs a processing context for a new transaction
    virtual IProcessContext* createProcessContext(IEspContext& espContext, const char* service, const char* method) const = 0;
    // Constructs a processing context from a persisted log blob
    virtual IProcessContext* restoreProcessContext(const char* logXML) const = 0;


    //
    virtual PhaseResult processPhase(IProcessContext& context, Phase phase) const = 0;
    inline  PhaseResult processPreflight(IProcessContext& context) const { return processPhase(context, Phase_Preflight); }
    inline  PhaseResult processRequest(IProcessContext& context) const { return processPhase(context, Phase_Request); }
    inline  PhaseResult processResponse(IProcessContext& context) const { return processPhase(context, Phase_Response); }
    inline  PhaseResult processLogManager(IProcessContext& context) const { return processPhase(context, Phase_LogManager); }
    inline  PhaseResult processLogAgent(IProcessContext& context) const { return processPhase(context, Phase_LogAgent); }
};

} // namespace EsdlScript

extern EsdlScript::IEnvironment* createEsdlScriptEnvironment();

#endif // ESDL_SCRIPT_CORE_HPP_
