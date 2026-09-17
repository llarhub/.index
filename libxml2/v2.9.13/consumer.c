#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

#include <stdio.h>
#include <string.h>

int main(void) {
    const char xml[] = "<?xml version=\"1.0\" encoding=\"windows-1252\"?>"
                       "<root><item>\x80</item></root>";
    LIBXML_TEST_VERSION
    xmlDocPtr doc = xmlReadMemory(xml, sizeof(xml) - 1, "consumer.xml", NULL, XML_PARSE_NONET);
    if (doc == NULL)
        return 1;
    xmlXPathContextPtr context = xmlXPathNewContext(doc);
    if (context == NULL) {
        xmlFreeDoc(doc);
        return 2;
    }
    xmlXPathObjectPtr result = xmlXPathEvalExpression(BAD_CAST "string(/root/item)", context);
    int status = result == NULL || result->type != XPATH_STRING ||
                 xmlStrcmp(result->stringval, BAD_CAST "\xe2\x82\xac") != 0;
    xmlXPathFreeObject(result);
    xmlXPathFreeContext(context);
    xmlFreeDoc(doc);
    xmlCleanupParser();
    if (status == 0)
        puts("libxml2 parser, XPath and encoding consumer passed");
    return status ? 3 : 0;
}
