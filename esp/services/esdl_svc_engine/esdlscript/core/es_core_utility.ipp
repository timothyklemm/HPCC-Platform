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

#ifndef _EsdlScriptCoreUtility_IPP_
#define _EsdlScriptCoreUtility_IPP_

#include "es_core_utility.hpp"
#include "xpp/XmlPullParser.h"
#include <functional>
#include <map>
#include <memory>
#include <string>

namespace EsdlScript
{

template <class TOwned>
class TOwner
{
public:
    operator Owned<TOwned>& () { return m_owned; }
    operator const Owned<TOwned>& () const { return m_owned; }
    TOwned* query() { return m_owned.get(); }
    TOwned* query() const { return m_owned.get(); }
    TOwned* get() { return LINK(m_owned.get()); }
    TOwned* get() const { return LINK(m_owned.get()); }
protected:
    Owned<TOwned> m_owned;
};

class CXMLParser : extends CInterface, implements IXMLParser
{
public:
    CXMLParser();
    ~CXMLParser();

    IMPLEMENT_IINTERFACE;

    void setInput(const StringBuffer& xml) override;
    bool next(int& entityType) override;
    bool next() override;
    void skip() override;
    int  currentEntityType() const override;
    bool hasMore() const override { return currentEntityType() != XmlPullParser::END_DOCUMENT; }
    bool atStartTag(const char* tag = nullptr) const override { return currentEntityType() == XmlPullParser::START_TAG && (isEmptyString(tag) || streq(tag, currentTag())); }
    bool atContent() const override { return currentEntityType() == XmlPullParser::CONTENT; }
    bool atEndTag(const char* tag = nullptr) const override { return currentEntityType() == XmlPullParser::END_TAG && (isEmptyString(tag) || streq(tag, currentTag())); }
    bool hasCurrentAttribute(const char* name) const override { return currentAttribute(name) != nullptr; }
    const char* currentTag() const override;
    const char* currentAttribute(const char* name) const override;
    const char* currentContent() const override;
    bool isCurrentContentSpace() const override;

private:
    using XmlPullParser = xpp::XmlPullParser;
    using StartTag = xpp::StartTag;
    using EndTag = xpp::EndTag;
    using Parser = std::unique_ptr<XmlPullParser>;
    Parser      m_parser;
    int         m_entityType = 0;
    StartTag    m_startTag;
    std::string m_content;
    EndTag      m_endTag;
    const char* m_tagName = nullptr;
};

class CPersistent
{
public:
    virtual StringBuffer& persist(StringBuffer& xml) const = 0;
    void                  restore(IXMLParser& parser, const char* tag, const char* counter = nullptr);
    IPTree*               persist(IPTree* parent) const;
    void                  restore(const IPTree* node, const char* tag, const char* counter = nullptr);
protected:
    virtual void          restoreInstance(IXMLParser& parser, const char* tag, size_t collectionCount) =  0;
    void                  restoring(const char* tag, size_t idx = 0, size_t cnt = 0) const;
};

#define IMPLEMENT_IPERSISTENT_COLLECTION(t, c) \
        IPTree*       persist(IPTree* parent) const override { return CPersistent::persist(parent); } \
        void          restore(const IPTree* node) override { CPersistent::restore(node, (t), (c)); } \
        StringBuffer& persist(StringBuffer& xml) const override; \
        void          restore(IXMLParser& parser) override { CPersistent::restore(parser, (t), (c)); } \
        void          restoreInstance(IXMLParser& parser, const char* tag, size_t collectionCount) override

#define IMPLEMENT_IPERSISTENT(t) IMPLEMENT_IPERSISTENT_COLLECTION(t, nullptr)

template <class TObject>
class TObjectFactory
{
public:
    using Creator = std::function<TObject*()>;
    enum Status
    {
        Success,
        Exists,
        Unregistered,
        Failed
    };

    virtual Status registerTag(const char* tag, Creator creator)
    {
        if (isEmptyString(tag))
            throw MakeStringException(0, "invalid empty factory tag during registration");

        Status result = Success;
        WriteLockBlock block(m_creatorLock);
        auto it = m_creatorMap.find(tag);

        if (it != m_creatorMap.end())
        {
            it->second = creator;
            result = Exists;
        }
        else
            m_creatorMap[tag] = creator;
        return result;
    }

    virtual bool isRegisteredTag(const char* tag) const
    {
        if (isEmptyString(tag))
            return false;

        ReadLockBlock block(m_creatorLock);
        auto it = m_creatorMap.find(tag);

        return it != m_creatorMap.end();
    }

    TObject* create(const char* tag, Status* status = nullptr) const
    {
        if (isEmptyString(tag))
            throw MakeStringException(0, "invalid empty factory tag during creation");

        Owned<TObject> object;
        bool found = false;
        { // limit the block scope
            ReadLockBlock block(m_creatorLock);
            auto it = m_creatorMap.find(tag);

            if (it != m_creatorMap.end())
            {
                found = true;
                object.setown((it->second)());
            }
        }

        if (!found)
        {
            if (status)
                *status = Unregistered;
        }
        else if (object.get() == nullptr)
        {
            if (status)
                *status = Failed;
        }
        else
            return object.getClear();
    }

    virtual Status eraseTag(const char* tag)
    {
        if (isEmptyString(tag))
            throw MakeStringException(0, "invalid empty factory tag during erasure");

        WriteLockBlock block(m_creatorLock);
        auto it = m_creatorMap.find(tag);

        if (it != m_creatorMap.end())
        {
            m_creatorMap.erase(it);
            return Success;
        }
        return Unregistered;
    }

private:
    using CreatorMap = std::map<std::string, Creator>;
    CreatorMap m_creatorMap;
    mutable ReadWriteLock m_creatorLock;
};


} // namespace EsdlScript

#endif // _EsdlScriptCoreUtility_IPP_
