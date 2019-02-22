/* sdebug extension for PHP */

#ifndef PHP_SDEBUG_H
#define PHP_SDEBUG_H

#include "php.h"
#include "ext/standard/info.h"
#include "zend_extensions.h"
#include "ext/standard/base64.h"
#include "zend_ini.h"
#include "zend_types.h"
#include "zend_hash.h"

#include <libxml/parser.h>
#include <libxml/parserInternals.h>
#include <libxml/tree.h>
#include <libxml/uri.h>
#include <libxml/xmlerror.h>
#include <libxml/xinclude.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/xpointer.h>
#include <libxml/xmlschemas.h>

#include "dbgp.h"
#include "sdebug_xml.h"

extern zend_module_entry sdebug_module_entry;
#define phpext_sdebug_ptr &sdebug_module_entry

#define PHP_SDEBUG_VERSION "0.0.1-alpha"
//#define PHP_SDEBUG_URL     "https://github.com/mabu233/sdebug"
//测试时发现windows上的phpstorm不传这个字符串就会响应不成功
#define PHP_SDEBUG_URL     "http://xdebug.org/dbgp/xdebug"

ZEND_BEGIN_MODULE_GLOBALS(sdebug)
    int sockfd;
    int status;
    char *lastcmd;
    char *transaction_id;
    int do_step;

    int remote_enable;
    char *remote_host;
    int remote_port;

    HashTable *breakpoint_list;
ZEND_END_MODULE_GLOBALS(sdebug)

ZEND_DECLARE_MODULE_GLOBALS(sdebug)

#define SG(v) (sdebug_globals.v)


#endif	/* PHP_SDEBUG_H */

