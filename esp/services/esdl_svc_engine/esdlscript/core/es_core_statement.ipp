/*##############################################################################

    HPCC SYSTEMS software Copyright (C) 2019 HPCC Systems®.

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

#ifndef _EsdlScriptCoreStatement_IPP_
#define _EsdlScriptCoreStatement_IPP_

#include "es_core_statement.hpp"
#include "es_core_factory.ipp"

namespace EsdlScript
{

class CStatementFactory : implements IStatementFactory, extends TFactory<IStatement>
{
public:
    CStatementFactory();
    ~CStatementFactory();

    // IInterface
    IMPLEMENT_IINTERFACE;
    // IFactory
    IMPLEMENT_IFACTORY(IStatement);
    // IStatementFactory
    bool initialize() override;
};

class CStatementLibrary : implements IStatementLibrary, extends CIInterface
{
public:
    CStatementLibrary();
    ~CStatementLibrary();


};

} // namespace EsdlScript

#endif // _EsdlScriptCoreStatement_IPP_
