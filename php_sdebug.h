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
# define phpext_sdebug_ptr &sdebug_module_entry

# define PHP_SDEBUG_VERSION "0.0.1-alpha"

ZEND_BEGIN_MODULE_GLOBALS(sdebug)
    int sockfd;
    int remote_enable;
    int status;
    char *lastcmd;
    char *transaction_id;
    int do_step;

    HashTable *breakpoint_list;
ZEND_END_MODULE_GLOBALS(sdebug)

ZEND_DECLARE_MODULE_GLOBALS(sdebug)

#define SG(v) (sdebug_globals.v)


#endif	/* PHP_SDEBUG_H */

