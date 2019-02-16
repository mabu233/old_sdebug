//
// Created by x on 2/6/19.
//

#ifndef SDEBUG_SDEBUG_XML_H
#define SDEBUG_SDEBUG_XML_H

xmlNodePtr sdebug_xml_new_node(const char *node_name);
void sdebug_xml_set_attr(xmlNodePtr* node, const char *attr_name, const char *value);
void sdebug_xml_set_content(xmlNodePtr* node, const char *content);
void sdebug_xml_add_child(xmlNodePtr* parent, xmlNodePtr* node);

char *sdebug_base64_decode(char *data);
char *sdebug_base64_encode(char *data, size_t len);


#endif //SDEBUG_SDEBUG_XML_H
