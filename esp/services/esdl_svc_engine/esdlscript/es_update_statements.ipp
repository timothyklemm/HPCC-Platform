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

#ifndef _EsUpdateStatements_IPP__
#define _EsUpdateStatements_IPP__

#include "esdl_script_core.ipp"

class CValueStatement : public CStatement
{
#if 0
    <xsdl:Value target="xpath" value="xpath"/>
    <xsdl:Value target="xpath">
        <xsdl:Value target="xpath" value="xpath"/>
        <!-- additional Value statements -->
    </xsdl:Value>
#endif
public:
    ~CValueStatement();
protected:
    CValueStatement();
    bool initializeSelf(ILoadContext& context) override;
    void validateSelf(ILoadContext& context) override;
    bool processSelf(IProcessContext& context, IParentContext* parentInfo, const char* readXPath, const char* writeXPath, Owned<IParentContext>& selfInfo) const override;
    virtual void applyValue(IProcessContext& context, IPTree* node, const char* xpath, const char* value) const = 0;
protected:
    StringBuffer          m_targetXPath;
    Owned<ICompiledXpath> m_compiledValueXPath;
};

class   CSetValueStatement : public CValueStatement
{
public:
    CSetValueStatement();
    ~CSetValueStatement();
protected:
    void applyValue(IProcessContext& context, IPTree* node, const char* xpath, const char* value) const override;
};

class CAppendValueStatement : public CValueStatement
{
public:
    CAppendValueStatement();
    ~CAppendValueStatement();
protected:
    void applyValue(IProcessContext& context, IPTree* node, const char* xpath, const char* value) const override;
};

#endif // _EsUpdateStatements_IPP__
