#ifndef _EsdlScriptCoreStatement_HPP_
#define _EsdlScriptCoreStatement_HPP_

#include "esp.hpp"
#include "esdlscript.hpp"
#include "es_core_context.hpp"
#include "es_core_factory.hpp"
#include "es_core_trace.hpp"
#include "es_core_utility.hpp"

namespace EsdlScript
{

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
    // will advance to the next sibling (or an ancestor).
    virtual bool initialize(ILoadContext& context) = 0;

    virtual const char* queryTag() const = 0;
    virtual const char* queryUID() const = 0;
    virtual Phase queryPhase() const = 0;

    // Called during transaction processing, executes  subclass-specific behaviors.
    virtual const char* queryReadCursor() const = 0;
    virtual const char* queryWriteCursor() const = 0;
//    virtual void process(IProcessContext& context, IParentContext* parentInfo, const char* readXPath, const char* writeXPath) const = 0;

    // Returns true if and only if the expected outcome of the statement is the production of a
    // Boolean value. Most statements will return false, while logical control statements will
    // return true.
    virtual bool isEvaluable() const = 0;
    // Returns the Boolean value produced by an evaluable statement, such as 'if' or 'when'.
    // Returns false always when the statement is not evaluable.
//    virtual bool evaluate(IProcessContext* context, IParentContext* parentInfo) const = 0;

    virtual bool isComparable() const = 0;
    virtual bool compare(const char* value, IProcessContext* context) const = 0;
};

interface IStatementFactory : extends IFactory<IStatement>
{
    virtual bool initialize() = 0;
};

DECLARE_IOWNER(StatementFactory);

interface IStatementLoadContext : extends IInterface, extends IStatementFactoryOwner, extends IXMLParserOwner
{

};

interface ILibrary : extends IInterface
{
    virtual bool load(IStatementLoadContext& context) = 0;
    virtual const IStatement* queryStatement(const char* uid) const = 0;
    virtual const IStatement* getStatement(const char* uid) const = 0;
};

} // namespace EsdlScript

#endif // _EsdlScriptCoreStatement_HPP_
