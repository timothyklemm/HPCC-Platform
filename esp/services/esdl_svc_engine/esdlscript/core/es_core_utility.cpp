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

#include "es_core_utility.ipp"
#include "es_core_trace.hpp"

namespace EsdlScript
{

CXMLParser::CXMLParser()
    : m_parser(new XmlPullParser())
{
}

CXMLParser::~CXMLParser()
{
}

void CXMLParser::setInput(const StringBuffer& xml)
{
    if (xml.length() > LONG_MAX)
        throw MakeStringException(-1, "XmlPullParser context initialization error [XML content too big (%u)]", xml.length());
    m_parser->setInput(xml, int(xml.length()));
    m_entityType = 0;
}
bool CXMLParser::next(int& entityType)
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
bool CXMLParser::next()
{
    int entityType;
    return next(entityType);
}
void CXMLParser::skip()
{
    m_parser->skipSubTree();
}
int CXMLParser::currentEntityType() const
{
    return m_entityType;
}
const char* CXMLParser::currentTag() const
{
    return m_tagName;
}
const char* CXMLParser::currentAttribute(const char* name) const
{
    if (m_tagName != nullptr)
        return m_startTag.getValue(name);
    return nullptr;
}
const char* CXMLParser::currentContent() const
{
    return m_content.c_str();
}
bool CXMLParser::isCurrentContentSpace() const
{
    return m_content.find_first_not_of(" \t\r\n") == std::string::npos;
}


IPTree* CPersistent::persist(IPTree* parent) const
{
    StringBuffer xml;
    IPTree* node = createPTreeFromXMLString(persist(xml));

    if (parent != nullptr && node != nullptr)
        parent->addPropTree(node->queryName(), LINK(node));
    return node;
}

void CPersistent::restore(IXMLParser& parser, const char* tag, const char* counter)
{
    StTraceContext tc(tag, "restore");

    if (isEmptyString(tag))
        Trace::internalError(0, "unable to restore without element tag");
    else if (!parser.atStartTag())
        Trace::internalError(0, "unable to restore '%s'; parser not at element", tag);
    else if (!parser.atStartTag(tag))
        Trace::internalError(0, "unable to restore '%s'; parser at '%s'", tag, parser.currentTag());
    else
    {
        size_t count = 0;

        restoring(tag);
        if (!isEmptyString(counter))
            parser.currentAttribute(counter, count);
        restoreInstance(parser, tag, count);
        while (!parser.atEndTag(tag) && parser.hasMore() && parser.next()) {}
        if (!parser.hasMore())
            Trace::internalError(0, "unexpected EOF");
    }
}

void CPersistent::restore(const IPTree* node, const char* tag, const char* counter)
{
    if (node != nullptr)
    {
        StringBuffer xml;
        Owned<IXMLParser> parser(new CXMLParser());

        toXML(node, xml);
        parser->setInput(xml);
        restore(*parser.get(), tag, counter);
    }
}

void CPersistent::restoring(const char* tag, size_t idx, size_t cnt) const
{
    VStringBuffer msg("restoring '%s' instance", tag);
    if (idx)
    {
        msg.appendf(" %zu", idx);
        if (cnt)
            msg.appendf(" of %zu", cnt);
    }
    Trace::developerProgress(0, msg);
}

} // namespace EsdlScript
