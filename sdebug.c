/* sdebug extension for PHP */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "php_sdebug.h"

PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("xdebug.remote_enable", "0", PHP_INI_ALL, OnUpdateLong, remote_enable, zend_sdebug_globals, sdebug_globals)
    STD_PHP_INI_ENTRY("xdebug.remote_host", "127.0.0.1", PHP_INI_ALL, OnUpdateString, remote_host, zend_sdebug_globals, sdebug_globals)
PHP_INI_END()

PHP_MINIT_FUNCTION(sdebug)
{
    REGISTER_INI_ENTRIES();

    SG(sockfd)         = 0;
    SG(status)         = DBGP_STATUS_STARTING;
    SG(lastcmd)        = (char *)malloc(MAX_CMD_NAME_LEN + 1);
    SG(transaction_id) = (char *)malloc(MAX_CMD_NAME_LEN + 1);
    SG(do_step)        = 0;

    SG(breakpoint_list) = (HashTable *)malloc(sizeof(HashTable));
    zend_hash_init(SG(breakpoint_list), 16, NULL, ZVAL_PTR_DTOR, 0);

    return SUCCESS;
}

/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(sdebug)
{
    CG(compiler_options) = CG(compiler_options) | (ZEND_COMPILE_EXTENDED_INFO);
    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(sdebug)
{
    php_info_print_table_start();
    php_info_print_table_header(2, "sdebug support", "enabled");
    php_info_print_table_end();
}
/* }}} */


/* {{{ sdebug_functions[]
 */
static const zend_function_entry sdebug_functions[] = {
    PHP_FE_END
};
/* }}} */

/* {{{ sdebug_module_entry
 */
zend_module_entry sdebug_module_entry = {
    STANDARD_MODULE_HEADER,
    "sdebug",                   /* Extension name */
    sdebug_functions,           /* zend_function_entry */
    PHP_MINIT(sdebug),          /* PHP_MINIT - Module initialization */
    NULL,                       /* PHP_MSHUTDOWN - Module shutdown */
    PHP_RINIT(sdebug),          /* PHP_RINIT - Request initialization */
    NULL,                       /* PHP_RSHUTDOWN - Request shutdown */
    PHP_MINFO(sdebug),          /* PHP_MINFO - Module info */
    PHP_SDEBUG_VERSION,         /* Version */
    STANDARD_MODULE_PROPERTIES
};
/* }}} */



#ifndef ZEND_EXT_API
# ifdef ZEND_WIN32
#  define ZEND_EXT_API __declspec(dllexport)
# elif defined(__GNUC__) && __GNUC__ >= 4
#  define ZEND_EXT_API __attribute__ ((visibility("default")))
# else
#  define ZEND_EXT_API
# endif
#endif

static int sdebug_zend_startup(zend_extension *extension)
{
    return zend_startup_module(&sdebug_module_entry);
}


void sdebug_statement_call(zend_execute_data *frame)
{
    int           lineno;
    char          *filename;
    zend_op_array *op_array = &frame->func->op_array;

    lineno     = EG(current_execute_data)->opline->lineno;
    filename   = (char*) ZSTR_VAL(op_array->filename);

    if (SG(remote_enable)) {
        if (!SG(sockfd)) {
            dbgp_init(filename);
        }
        if (SG(do_step)) {
            SG(do_step) = 0;
            dbgp_breakpoint_handler(filename, lineno, BREAKPOINT_TYPE_STEP);
            return;
        }
        if (SG(breakpoint_list)->nNumUsed) {
            char          key[strlen(filename) + 8];
            sprintf((char *)key, "%s:%d", filename, lineno);
            if (zend_hash_str_exists(SG(breakpoint_list), key, strlen(key))) {
                dbgp_breakpoint_handler(filename, lineno, BREAKPOINT_TYPE_BREAK);
            }
        }
    }
}


ZEND_EXT_API zend_extension_version_info extension_version_info = { ZEND_EXTENSION_API_NO, (char*) ZEND_EXTENSION_BUILD_ID };

ZEND_EXT_API zend_extension zend_extension_entry = {
        (char*) "Sdebug",
        (char*) PHP_SDEBUG_VERSION,
        (char*) "mabu233",
        (char*) PHP_SDEBUG_URL,
        (char*) "",
        sdebug_zend_startup,
        NULL,
        NULL,           /* activate_func_t */
        NULL,           /* deactivate_func_t */
        NULL,           /* message_handler_func_t */
        NULL,           /* op_array_handler_func_t */
        sdebug_statement_call, /* statement_handler_func_t */
        NULL,           /* fcall_begin_handler_func_t */
        NULL,           /* fcall_end_handler_func_t */
        NULL,           /* op_array_ctor_func_t */
        NULL,           /* op_array_dtor_func_t */
        STANDARD_ZEND_EXTENSION_PROPERTIES
};