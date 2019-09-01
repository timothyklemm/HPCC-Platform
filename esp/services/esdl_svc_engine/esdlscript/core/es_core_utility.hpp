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

#ifndef _EsdlScriptCoreUtility_HPP_
#define _EsdlScriptCoreUtility_HPP_

#include "esp.hpp"
#include "jptree.hpp"
#include "jstring.hpp"
#include "tokenserialization.hpp"

namespace EsdlScript
{

// Specifies a standard interface for to access owned member instances. It provides const and non-
// const access to the instance, with or without adding a reference.
#define DECLARE_IOWNER(token) \
interface I ## token ## Owner \
{ \
    virtual I ## token* query ## token() const = 0; \
    virtual I ## token* query ## token() = 0; \
    virtual I ## token* get ## token() const = 0; \
    virtual I ## token* get ## token() = 0; \
}

interface IXMLParser : extends IInterface
{
    virtual void setInput(const StringBuffer& xml) = 0;
    virtual bool next(int& entityType) = 0;
    virtual bool next() = 0;
    virtual void skip() = 0;
    virtual int  currentEntityType() const = 0;
    virtual bool hasMore() const = 0;
    virtual bool atStartTag(const char* tag = nullptr) const = 0;
    virtual bool atContent() const = 0;
    virtual bool atEndTag(const char* tag = nullptr) const = 0;
    virtual bool hasCurrentAttribute(const char* name) const = 0;
    virtual const char* currentTag() const = 0;
    virtual const char* currentAttribute(const char* name) const = 0;
    virtual const char* currentContent() const = 0;
    virtual bool isCurrentContentSpace() const = 0;

    template <typename TValue>
    bool currentAttribute(const char* name, TValue& effectiveValue, const TValue& defaultValue = TValue()) const;
};

DECLARE_IOWNER(XMLParser);

// Specifies a standard serialization interface to be used by derived classes to save their content
// into either a text or tree representation of XML.
interface IPersistent
{
    virtual StringBuffer& persist(StringBuffer& xml) const = 0;
    virtual IPTree* persist(IPTree* parent) const = 0;
    virtual void restore(IXMLParser& parser) = 0;
    virtual void restore(const IPTree* node) = 0;
};

//------------------------------------------------------------------------------

template <typename TValue>
inline bool IXMLParser::currentAttribute(const char* name, TValue& effectiveValue, const TValue& defaultValue) const
{
    const char* rawValue = currentAttribute(name);
    TokenDeserializer deserializer;
    bool exists = !isEmptyString(rawValue) && deserializer(rawValue, effectiveValue) == Deserialization_SUCCESS;

    if (!exists)
        effectiveValue = defaultValue;
    return exists;
}

} // namespace EsdlScript

#endif // _EsdlScriptCoreUtility_HPP_
