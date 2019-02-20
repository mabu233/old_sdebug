//
// Created by x on 2/4/19.
//

#ifndef SDEBUG_DBGP_H
#define SDEBUG_DBGP_H

#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/un.h>
#include <arpa/inet.h>

#define MAX_CMD_NAME_LEN 24
#define READ_BUFFER_SIZE 512

enum dbgp_status_code {
    DBGP_STATUS_STARTING = 0,
    DBGP_STATUS_STOPPING,
    DBGP_STATUS_STOPPED,
    DBGP_STATUS_RUNNING,
    DBGP_STATUS_BREAK
};

#define DBGP_STATUS_MSG(status_code) dbgp_status_string[status_code]

enum dbgp_breakpoint_type {
    BREAKPOINT_TYPE_BREAK = 1,
    BREAKPOINT_TYPE_STEP  = 2
};

typedef struct {
    char name[MAX_CMD_NAME_LEN + 1];
    HashTable* args;
} dbgp_arg;

//!!!
typedef struct {
    char *buffer;
    int  index;
    int  size;
} Dbgp_Buffer;


#define INIT_DBGP_ARG(name) do { \
    name = (dbgp_arg *)malloc(sizeof(dbgp_arg)); \
    name->args = (HashTable *)emalloc(sizeof(HashTable)); \
    zend_hash_init(name->args, 16, NULL, ZVAL_PTR_DTOR, 0); \
} while (0)
#define FREE_DBGP_ARG(arg)   \
    zend_array_destroy(arg->args); \
    free(arg)

#define DBGP_FUNC_PARAMETERS    dbgp_arg* args, xmlNodePtr* retval
#define DBGP_FUNC(name)         static void sdebug_dbgp_handle_##name(DBGP_FUNC_PARAMETERS)
#define DBGP_FUNC_ENTRY(name)   { #name, sdebug_dbgp_handle_##name }

typedef struct {
    char name[MAX_CMD_NAME_LEN + 1];
    void (*handler)(DBGP_FUNC_PARAMETERS);
} commands_handler;

#define DBGP_GET_OPTVAL(arg, opt_name) \
    (zend_hash_str_exists(arg->args, ZEND_STRL(opt_name)) ? \
    Z_STRVAL_P(zend_hash_str_find(arg->args, ZEND_STRL(opt_name))) : \
    NULL)



int dbgp_init(const char *filename);
void dbgp_breakpoint_handler(const char *filename, int lineno, int breakpoint_type);


#endif //SDEBUG_DBGP_H
