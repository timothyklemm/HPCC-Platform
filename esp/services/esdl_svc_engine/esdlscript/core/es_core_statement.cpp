#include "es_core_statement.ipp"

namespace EsdlScript
{

CStatementFactory::CStatementFactory()
    : TFactory<IStatement>()
{
}

CStatementFactory::~CStatementFactory()
{
}

bool CStatementFactory::initialize()
{
    // TODO: add statement registrations
    return true;
}

} // namespace EsdlScript
