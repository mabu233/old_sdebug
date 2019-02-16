//
// Created by x on 2/6/19.
//

#include "php_sdebug.h"

xmlNodePtr sdebug_xml_new_node(const char *node_name)
{
    return xmlNewNode(NULL, (xmlChar *)node_name);
}

void sdebug_xml_set_attr(xmlNodePtr* node, const char *attr_name, const char *value)
{
    xmlNewProp(*node, (xmlChar *)attr_name, (xmlChar *)value);
}

void sdebug_xml_set_content(xmlNodePtr* node, const char *content)
{
    xmlNodeSetContent(*node, (xmlChar *)content);
}

void sdebug_xml_add_child(xmlNodePtr* parent, xmlNodePtr* node)
{
    xmlAddChild(*parent, *node);
}

char *sdebug_base64_decode(char *data)
{
    zend_string *tmp;
    char *retval;

    tmp = php_base64_decode(data, strlen(data));
    retval = (char *)malloc(tmp->len + 1);
    memcpy(retval, tmp->val, tmp->len);
    retval[tmp->len] = 0;

    zend_string_release(tmp);

    return retval;
}

char *sdebug_base64_encode(char *data, size_t len)
{
    zend_string *tmp;
    char *retval;

    tmp = php_base64_encode(data, len);
    retval = (char *)malloc(tmp->len + 1);
    memcpy(retval, tmp->val, tmp->len);
    retval[tmp->len] = 0;

    zend_string_release(tmp);

    return retval;
}