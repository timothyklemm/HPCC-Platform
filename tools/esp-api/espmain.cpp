/*##############################################################################

    Copyright (C) 2024 HPCC Systems®.

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
#include "jlog.hpp"
#include "espshell.hpp"
#include "jscm.hpp"
#include <iostream>
using namespace std;
int main(int argc, const char* argv[])
{
    try
    {
        InitModuleObjects();
        queryStderrLogMsgHandler()->setMessageFields(0);
        queryLogMsgManager()->removeMonitor(queryStderrLogMsgHandler());
        EspShell myshell(argc,argv);
        if(myshell.parseCmdOptions())
        {
            return myshell.callService();
        }
    }
    catch(IException *e)
    {
        StringBuffer msg;
        e->errorMessage(msg);
        cerr << msg.str();
        e->Release();
    }
    return 1;
}
