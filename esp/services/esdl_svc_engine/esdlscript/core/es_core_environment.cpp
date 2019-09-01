#include "esdlscript.hpp"
#include "jptree.hpp"

namespace EsdlScript
{

void Script::add(const IPTree* tree, const char* xpath, const char* service, const char* method)
{
    if (nullptr == tree)
        throw MakeStringException(-1, "EsdlScript environment initialization error [invalid parameter]");

    Fragment fragment;
    if (isEmptyString(xpath))
    {
        if (tree->hasChildren())
            toXML(tree, fragment.content);
        else
        {
            Owned<IAttributeIterator> attrs = tree->getAttributes();
            auto content = tree->queryProp(".");

            fragment.content << '<' << tree->queryName();
               ForEach(*attrs)
                fragment.content << ' ' << attrs->queryName() << "=\"" << attrs->queryValue() << '"';
               if (isEmptyString(content))
                   fragment.content << "/>";
               else
                   fragment.content << '>' << content << "</" << tree->queryName() << '>';
        }
        fragment.service = service;
        fragment.method = method;
        push_back(fragment);
    }
    else
    {
        Owned<IPTreeIterator> nodes = tree->getElements(xpath);

        ForEach(*nodes)
        {
            add(&nodes->query(), "", service, method);
        }
    }
}



} // namespace EsdlScript
