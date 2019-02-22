//
// Created by x on 2/4/19.
//
#include "php_sdebug.h"

static char*        read_command_data();
static void         send_message(xmlNodePtr* message);
static dbgp_arg*    parse_arg(const char *command_str, dbgp_arg *args);
static int          command_exec(dbgp_arg *args, xmlNodePtr* retval);
static void         dbgp_cmdloop();
void var_export_xml_node(zval *val, xmlNodePtr *node);

DBGP_FUNC(feature_set);
DBGP_FUNC(status);
DBGP_FUNC(step_into);
DBGP_FUNC(step_over);
DBGP_FUNC(step_out);
DBGP_FUNC(breakpoint_set);
DBGP_FUNC(stack_get);
DBGP_FUNC(run);
DBGP_FUNC(context_names);
DBGP_FUNC(context_get);
DBGP_FUNC(breakpoint_remove);
DBGP_FUNC(stop);
DBGP_FUNC(eval);

static commands_handler commands_handlers[] = {
    DBGP_FUNC_ENTRY(feature_set),
    DBGP_FUNC_ENTRY(status),
    DBGP_FUNC_ENTRY(step_into),
    DBGP_FUNC_ENTRY(step_over),
    DBGP_FUNC_ENTRY(step_out),
    DBGP_FUNC_ENTRY(breakpoint_set),
    DBGP_FUNC_ENTRY(stack_get),
    DBGP_FUNC_ENTRY(run),
    DBGP_FUNC_ENTRY(context_names),
    DBGP_FUNC_ENTRY(context_get),
    DBGP_FUNC_ENTRY(breakpoint_remove),
    DBGP_FUNC_ENTRY(stop),
    DBGP_FUNC_ENTRY(eval),
    {{0}, NULL}
};

const char *dbgp_status_string[5] = {
    "starting",
    "stopping",
    "stopped",
    "running",
    "break"
};

int breakpoint_id = 1000000;

int dbgp_init(const char *filename)
{
    char init_str_tmp[512 + strlen(filename)];
    char init_str[512 + strlen(filename)];
    char *env_str = getenv("XDEBUG_CONFIG");
    char *idekey = NULL, *ptr_1, *ptr_2;
    int  idekey_len;
    if (env_str && (ptr_1 = strstr(env_str, "idekey="))) {
        ptr_2 = strchr(ptr_1, ';');
        idekey_len = (ptr_2 ? ptr_2 : env_str + strlen(env_str)) - (ptr_1 + 7);
        idekey = (char *)malloc(idekey_len + 1);
        strncpy(idekey, ptr_1 + 7, idekey_len);
        idekey[idekey_len] = '\0';
    } else {
        SG(remote_enable) = 0;
        return 0;
    }

    sprintf(init_str_tmp, "<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>"
             "<init xmlns=\"urn:debugger_protocol_v1\" "
             "xmlns:sdebug=\"%s\" "
             "appid=\"%d\" "
             "idekey=\"%s\" "
             "language=\"PHP\" "
             "sdebug:language_version=\"%s\" "
             "protocol_version=\"1.0\" "
             "fileuri=\"file://%s\">"
             "<engine version=\"%s\">"
             "Sdebug"
             "</engine>"
             "</init>", PHP_SDEBUG_URL, getpid(), idekey, PHP_VERSION, filename, PHP_SDEBUG_VERSION);
    int init_str_len = strlen(init_str_tmp);
    sprintf(init_str, "%d", init_str_len);
    memcpy(init_str + strlen(init_str) + 1, init_str_tmp, init_str_len + 1);


    struct sockaddr_in sin = {};
    int sockfd;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = inet_addr(SG(remote_host));
    sin.sin_port = htons(SG(remote_port));
    if(connect(sockfd,(struct sockaddr*)&sin,sizeof(struct sockaddr)) == -1){
        printf("connect fail[%d]\n", errno);
        SG(remote_enable) = 0;
    }else {
        SG(sockfd) = sockfd;
//        fcntl(SG(sockfd), F_SETFL, O_NONBLOCK);
        send(SG(sockfd), init_str, init_str_len + strlen(init_str) + 2, 0);
        dbgp_cmdloop();
    }
    if (idekey) {
        free(idekey);
    }
    return 0;
}

static char* read_command_data(Dbgp_Buffer *buffer)
{
    char *buf = buffer->buffer + buffer->index;
    int buf_len = buffer->size, tmp_len = 0;
    char delim  = '\0';

    while (buf_len == 0 || buf[buf_len - 1] != delim)
    {
        tmp_len = recv(SG(sockfd), buf + buf_len, READ_BUFFER_SIZE, 0);
        if (tmp_len > 0) {
            buf_len += tmp_len;
        } else if (tmp_len == -1) {
            printf("recv[%d]\n", errno);
        } else {
            printf("recv[0]\n");
            return NULL;
        }
    }

    int ptr;
    if ((ptr = strlen(buf)) < buf_len - 1) {
        buffer->index += ptr + 1;
        buffer->size = buf_len - (ptr + 1);
    }else {
        buffer->index = 0;
        buffer->size = 0;
    }
    return buf;
}

static void send_message(xmlNodePtr* message)
{
    xmlChar     *strval;
    int         strval_len;
    xmlDocPtr   docp;
    char        *buffer;

    docp = xmlNewDoc("1.0");
    xmlDocSetRootElement(docp, *message);
    xmlDocDumpFormatMemoryEnc(docp, &strval, &strval_len, (xmlChar *)"iso-8859-1", 1);

    buffer = (char *)malloc(strval_len + 12);
    memset(buffer, 0, strval_len + 12);

//    printf("libxml(%d) %s\n", strval_len, (char *)strval);

    sprintf(buffer, "%d", strval_len);
    memcpy(buffer + strlen(buffer) + 1, (char *)strval, strval_len);

    //10\0abcdeabcde\0
    send(SG(sockfd), buffer, strlen(buffer) + strval_len + 2, 0);

    xmlFreeDoc(docp);
    xmlFree(strval);
    free(buffer);
}

static dbgp_arg* parse_arg(const char *command_str, dbgp_arg *args)
{
    char ch;
    char *ptr;

    ptr = strchr(command_str, ' ');
    if (!ptr || ptr - command_str > MAX_CMD_NAME_LEN)
    {
        printf("command_name err!\n");
        return NULL;
    }
    memcpy((char *)args->name, command_str, ptr - command_str);
    args->name[ptr - command_str] = 0;

    zval pData;
    char *pos_ptr, *tmp_ptr;
    char opt_name[16] = {0};
    char *opt_val = (char *)malloc(strlen(command_str));  //!!!
    int  state = 0;
    while(ch = *ptr)
    {
        switch (state) {
            case 0:
                if (ch == '-') {
                    state = 1;
                    tmp_ptr = opt_name;
                    continue;
                }
                break;
            case 1:
            case 2:
                pos_ptr = strchr(ptr, ' ');
                if (!pos_ptr) {
                    pos_ptr = (char *)(command_str + strlen(command_str));
                }
                memcpy((char *)tmp_ptr, ptr , pos_ptr - ptr);
                tmp_ptr[pos_ptr - ptr] = 0;
                ptr = pos_ptr;
                if (state == 1) {
                    tmp_ptr = opt_val;
                    state = 2;
                } else {
                    ZVAL_STRING(&pData, (char *)opt_val);
                    zend_hash_str_add(args->args, (char *)opt_name, strlen(opt_name), &pData);
                    state = 0;
                }
                break;
        }
        if (*ptr) ptr++;
    }
    free(opt_val);
//    zval_ptr_dtor(&pData); //!!!
    return args;
}

static int command_exec(dbgp_arg *args, xmlNodePtr* retval)
{
    char *transaction_id;
    commands_handler *ptr = commands_handlers;

    while (ptr->handler)
    {
        if (strcmp(ptr->name, args->name) == 0) {
            break;
        }
        ptr++;
    }
    if (ptr->handler)
    {
        ptr->handler(args, retval);
        transaction_id = DBGP_GET_OPTVAL(args, "-i");
        if (transaction_id) {
            memcpy(SG(transaction_id), transaction_id, strlen(transaction_id) + 1);
            sdebug_xml_set_attr(retval, "transaction_id", transaction_id);
        }
        memcpy(SG(lastcmd), args->name, strlen(args->name) + 1);
        sdebug_xml_set_attr(retval, "command", args->name);
    } else {
        printf("command %s is not exists!\n", args->name);
    }
    if (SG(do_step) || SG(status) == DBGP_STATUS_RUNNING) {
        return -1;
    }
    return 0;
}

static void dbgp_cmdloop()
{
    int         ret = 0;
    char        *buf;
    dbgp_arg    *args;
    xmlNodePtr  retval;
    Dbgp_Buffer buffer;

    //!!!
    buffer.buffer = (char *)malloc(8192 * 8);
    memset(buffer.buffer, 0, 8192 * 8);
    buffer.index = 0;
    buffer.size = 0;

    do {
        buf = read_command_data(&buffer);
        if (!buf) {
            printf("lineno:[%d], read data is error[%d]!\n", __LINE__, errno);
            free(buffer.buffer);
            return;
        }
        INIT_DBGP_ARG(args);
        if (!parse_arg(buf, args)) {
            printf("lineno:[%d], parse_arg is error!\n", __LINE__);
            FREE_DBGP_ARG(args);
            free(buffer.buffer);
            return;
        }
        retval = sdebug_xml_new_node("response");
        sdebug_xml_set_attr(&retval, "xmlns", "urn:debugger_protocol_v1");
        sdebug_xml_set_attr(&retval, "xmlns:sdebug", PHP_SDEBUG_URL);

        ret = command_exec(args, &retval);

        if (ret == 0) {
            send_message(&retval);
        } else {
            xmlFreeNode(retval);
        }
        FREE_DBGP_ARG(args);
    } while(ret == 0);
    free(buffer.buffer);
}

void dbgp_breakpoint_handler(const char *filename, int lineno, int breakpoint_type)
{
    xmlNodePtr response, message;
    char *tmp_filename, *tmp_lineno;

    SG(status) = DBGP_STATUS_BREAK;

    tmp_filename = (char *)malloc(strlen(filename) + sizeof("file://"));
    sprintf(tmp_filename, "file://%s", filename);
    tmp_lineno = (char *)malloc(8);
    sprintf(tmp_lineno, "%d", lineno);

    response = sdebug_xml_new_node("response");
    sdebug_xml_set_attr(&response, "xmlns", "urn:debugger_protocol_v1");
//    sdebug_xml_set_attr(&response, "xmlns:sdebug", "http://xdebug.org/dbgp/xdebug");
    sdebug_xml_set_attr(&response, "xmlns:sdebug", PHP_SDEBUG_URL);

    if (SG(lastcmd)[0] != '\0' && SG(transaction_id)[0] != '\0') {
        sdebug_xml_set_attr(&response, "command", SG(lastcmd));
        sdebug_xml_set_attr(&response, "transaction_id", SG(transaction_id));
    }
    sdebug_xml_set_attr(&response, "status", DBGP_STATUS_MSG(SG(status)));
    sdebug_xml_set_attr(&response, "reason", "ok");

    message = sdebug_xml_new_node("sdebug:message");
    sdebug_xml_set_attr(&message, "filename", tmp_filename);
    sdebug_xml_set_attr(&message, "lineno", tmp_lineno);

    sdebug_xml_add_child(&response, &message);

    send_message(&response);

    free(tmp_filename);
    free(tmp_lineno);

    dbgp_cmdloop();
}

xmlNodePtr stack_get(xmlNodePtr *retval)
{
    xmlNodePtr stack[8192]; //在大我也没办法了QAQ
    int        i = 0;
    char       *filename, *tmp_filename, *tmp_lineno;

    tmp_filename = (char *)malloc(1024);
    tmp_lineno   = (char *)malloc(8);

    zend_execute_data *ptr = EG(current_execute_data);
    while (ptr && i < 8192) {
        stack[i] = sdebug_xml_new_node("stack");
        if(ptr->func) {
            if (ptr->func->common.function_name) {
                sdebug_xml_set_attr(&stack[i], "where", ptr->func->common.function_name->val);
            } else {
                sdebug_xml_set_attr(&stack[i], "where", "{main}");
            }
            if (ptr->func->type == 2) {
                sprintf(tmp_lineno, "%d", ptr->opline->lineno);
                sdebug_xml_set_attr(&stack[i], "lineno", tmp_lineno);
                sprintf(tmp_filename, "file://%s", ptr->func->op_array.filename->val);
                sdebug_xml_set_attr(&stack[i], "filename", tmp_filename);
            } else {
                sdebug_xml_set_attr(&stack[i], "lineno", "0");
                sdebug_xml_set_attr(&stack[i], "filename", "internal function]");
            }
        } else {
            sdebug_xml_set_attr(&stack[i], "lineno", "0");
            sdebug_xml_set_attr(&stack[i], "where", "???");
            sdebug_xml_set_attr(&stack[i], "filename", "???");
        }
        ptr = ptr->prev_execute_data;
        i++;
    }

    for (int j = 0; j < i; j++) {
        sprintf(tmp_lineno, "%d", j);
        sdebug_xml_set_attr(&stack[j], "level", tmp_lineno);
        sdebug_xml_set_attr(&stack[j], "type", "file");
        sdebug_xml_add_child(retval, &stack[j]);
    }
    free(tmp_filename);
    free(tmp_lineno);
}

DBGP_FUNC(feature_set)
{
    //feature_set -i 1 -n show_hidden -v 1
    char *opt_val = DBGP_GET_OPTVAL(args, "-n");
    if (!opt_val) {
        printf("opt_val is null\n");
    }
    if (strcmp(opt_val, "show_hidden") == 0) {

    } else {

    }

    sdebug_xml_set_attr(retval, "feature", opt_val);
    sdebug_xml_set_attr(retval, "success", "1");
}

DBGP_FUNC(status)
{
    //status -i 5
    sdebug_xml_set_attr(retval, "status", DBGP_STATUS_MSG(SG(status)));
    sdebug_xml_set_attr(retval, "reason", "ok");
}

DBGP_FUNC(step_into)
{
    //step_into -i 6
    SG(do_step) = 1;
}

DBGP_FUNC(step_out)
{
    //!!!
    SG(do_step) = 1;
}

DBGP_FUNC(step_over)
{
    //!!!
    SG(do_step) = 1;
}

DBGP_FUNC(breakpoint_set)
{
    //breakpoint_set -i 7 -t line -f file:///home/x/php-src-php-7.3.1/ext/sdebug/1.php -n 18
    zval pData;
    char *key, *resolved_path;
    char id[10];

    char *filename = DBGP_GET_OPTVAL(args, "-f");
    char *lineno   = DBGP_GET_OPTVAL(args, "-n");
    if (!filename || !lineno) {
        printf("filename and lineno is not null!\n");
        return;
    }
    resolved_path = realpath(filename + 7, NULL);
    if (!resolved_path) {
        printf("realpath error, lineno: %d, error: %d\n", __LINE__, errno);
        return;
    }
    key = (char *)malloc(strlen(resolved_path) + 10);
    sprintf(key, "%s:%s", resolved_path, lineno);

    ZVAL_LONG(&pData, breakpoint_id);
    zend_hash_str_add(SG(breakpoint_list), key, strlen(key), &pData);
    sprintf((char *)id, "%d", breakpoint_id++);
    sdebug_xml_set_attr(retval, "id", id);

    zval_ptr_dtor(&pData);
    free(key);
    free(resolved_path);
}

DBGP_FUNC(breakpoint_remove)
{
    char *id   = DBGP_GET_OPTVAL(args, "-d");
    zend_string *key;
    zval *val;

    if (!id) {
        printf("breakpoint_id is not null!\n");
        return;
    }

    ZEND_HASH_FOREACH_STR_KEY_VAL(SG(breakpoint_list), key, val) {
        if(Z_LVAL_P(val) == atoi(id)) {
            printf("breakpoint_remove[%s] ok!\n", key->val);
            zend_hash_del_ind(SG(breakpoint_list), key);
            return;
        }
    } ZEND_HASH_FOREACH_END();
}

DBGP_FUNC(stack_get)
{
    //stack_get -i 8
    stack_get(retval);
}

DBGP_FUNC(run)
{
    //run -i 9
    SG(status) = DBGP_STATUS_RUNNING;
}

DBGP_FUNC(stop)
{
    //stop -i 22
    SG(status) = DBGP_STATUS_STOPPING;
    exit(0);
}

DBGP_FUNC(eval)
{
    //eval -i 23 -- base64
    xmlNodePtr property = sdebug_xml_new_node("property");
    char *eval_str = DBGP_GET_OPTVAL(args, "--");
    eval_str = sdebug_base64_decode(eval_str);
//    printf("eval: %s \n", eval_str);

    zval res;
    zend_eval_string(eval_str, &res, (char*)"sdebug-eval");

    var_export_xml_node(&res, &property);

    sdebug_xml_add_child(retval, &property);

    free(eval_str);
    zval_ptr_dtor(&res);
}

DBGP_FUNC(context_names)
{
    //context_names -i 11
    xmlNodePtr context = sdebug_xml_new_node("context");
    sdebug_xml_set_attr(retval, "name", "Locals");
    sdebug_xml_set_attr(retval, "id", "0");
    sdebug_xml_add_child(retval, &context);
}

DBGP_FUNC(context_get)
{
    //context_get -i 20 -d 0 -c 0
    zend_execute_data *ex = EG(current_execute_data);
    char *context_id;
    int  depth;

    if (!(context_id = DBGP_GET_OPTVAL(args, "-c"))) {
        printf("context_id is null\n");
        return;
    }
    if (!(DBGP_GET_OPTVAL(args, "-d"))) {
        printf("depth is null\n");
        return;
    }

    depth = atoi(DBGP_GET_OPTVAL(args, "-d"));
    while (ex && depth--) {
        ex = ex->prev_execute_data;
    }
    if (!ex) {
        printf("depth is too big\n");
        return;
    }

    char tmp_name[64], tmp_fullname[64]; //!!!
    xmlNodePtr property;
    if (strcmp(context_id, "0") == 0) {
        int cv_count          = ex->func->op_array.last_var;
        int callFrameSize     = (sizeof(zend_execute_data) + sizeof(zval) - 1) / sizeof(zval);
        zval *cv_ptr          = ((zval *)ex) + callFrameSize;

        for (int i = 0; i < cv_count; i++) {
            property = sdebug_xml_new_node("property");
            sprintf((char *)tmp_name, "$%s", ex->func->op_array.vars[i]->val);
            sprintf((char *)tmp_fullname, "$%s", ex->func->op_array.vars[i]->val);
            sdebug_xml_set_attr(&property, "name", tmp_name);
            sdebug_xml_set_attr(&property, "fullname", tmp_fullname);
            var_export_xml_node(cv_ptr + i, &property);
            sdebug_xml_add_child(retval, &property);
        }
    }
    sdebug_xml_set_attr(retval, "context", context_id);
}

void var_export_xml_node(zval *val, xmlNodePtr *node)
{
    char tmp_name[64], tmp_fullname[64], tmp_value[1024];

    if (Z_TYPE_P(val) == IS_INDIRECT) {
        val = val->value.zv;
	}
    if (Z_TYPE_P(val) == IS_REFERENCE) {
        val = &(val->value.ref->val);
    }
    switch (Z_TYPE_P(val)) {
        case IS_UNDEF:
            sdebug_xml_set_attr(node, "type", "uninitialized");
            break;
        case IS_TRUE:
            sdebug_xml_set_attr(node, "type", "bool");
            sdebug_xml_set_content(node, "1");
            break;
        case IS_FALSE:
            sdebug_xml_set_attr(node, "type", "bool");
            sdebug_xml_set_content(node, "1");
            break;
        case IS_NULL:
            sdebug_xml_set_attr(node, "type", "null");
            break;
        case IS_LONG:
            sdebug_xml_set_attr(node, "type", "int");
            sprintf((char *)tmp_value, "%d", Z_LVAL_P(val));
            sdebug_xml_set_content(node, tmp_value);
            break;
        case IS_DOUBLE:
            sdebug_xml_set_attr(node, "type", "double");
            sprintf((char *)tmp_value, "%.*G", (int) EG(precision), Z_DVAL_P(val));
            sdebug_xml_set_content(node, tmp_value);
            break;
        case IS_STRING: {
            char *base64_str;
            sdebug_xml_set_attr(node, "type", "string");
            sdebug_xml_set_attr(node, "encoding", "base64");
            sprintf((char *)tmp_value, "%d", Z_STRLEN_P(val));
            sdebug_xml_set_attr(node, "size", tmp_value);
            base64_str = sdebug_base64_encode(Z_STRVAL_P(val), Z_STRLEN_P(val));
            sdebug_xml_set_content(node, base64_str);
            free(base64_str);
            break;
        }    
        case IS_ARRAY: {
            xmlNodePtr child;
            zend_ulong num;
            zend_string *key;
            zval *z_val;

            sdebug_xml_set_attr(node, "type", "array");
            sdebug_xml_set_attr(node, "children", Z_ARRVAL_P(val)->nNumOfElements > 0 ? "1" : "0");
            if (ZEND_HASH_GET_APPLY_COUNT(Z_ARRVAL_P(val)) < 1) {
                sprintf((char *) tmp_value, "%d", Z_ARRVAL_P(val)->nNumOfElements);
                sdebug_xml_set_attr(node, "numchildren", tmp_value);
                sdebug_xml_set_attr(node, "page", "0");
                sdebug_xml_set_attr(node, "pagesize", "100");

                ZEND_HASH_INC_APPLY_COUNT(Z_ARRVAL_P(val));
                ZEND_HASH_FOREACH_KEY_VAL_IND(Z_ARRVAL_P(val), num, key, z_val)
                {
                    child = sdebug_xml_new_node("property");
                    if(key) {
                        sdebug_xml_set_attr(&child, "name", key->val);
                        sdebug_xml_set_attr(&child, "fullname", key->val);
                    }else {
                        sprintf((char *) tmp_value, "%d", num);
                        sdebug_xml_set_attr(&child, "name", tmp_value);
                        sdebug_xml_set_attr(&child, "fullname", tmp_value);
                    }
                    var_export_xml_node(z_val, &child);
                    sdebug_xml_add_child(node, &child);
                }
                ZEND_HASH_FOREACH_END();
                ZEND_HASH_DEC_APPLY_COUNT(Z_ARRVAL_P(val));
            } else {
                sdebug_xml_set_attr(node, "recursive", "1");
            }
            break;
        }
        case IS_OBJECT: {
            xmlNodePtr child;
            zend_ulong num;
            zend_string *key;
            zval *z_val;
            int children = 0;
            zend_class_entry *tmp_ce;
            zend_property_info *zpi_val;
            HashTable *ht;
            const char *cls_name, *tmp_prop_name;
            size_t      tmp_prop_name_len;

            sdebug_xml_set_attr(node, "type", "object");
            sdebug_xml_set_attr(node, "classname", Z_OBJCE_P(val)->name->val);
            sdebug_xml_set_attr(node, "page", "0");
            sdebug_xml_set_attr(node, "pagesize", "100");

            if (ZEND_HASH_GET_APPLY_COUNT(&(Z_OBJCE_P(val)->properties_info)) < 1) {
                tmp_ce = zend_fetch_class(Z_OBJCE_P(val)->name, ZEND_FETCH_CLASS_DEFAULT);
                ZEND_HASH_INC_APPLY_COUNT(&tmp_ce->properties_info);
                ZEND_HASH_FOREACH_PTR(&(tmp_ce->properties_info), zpi_val)
                {
                    if((zpi_val->flags & ZEND_ACC_STATIC) == 0){
                        continue;
                    }
                    z_val = &tmp_ce->static_members_table[zpi_val->offset];
                    children++;
                    child = sdebug_xml_new_node("property");
                    if(zpi_val) {
                        sdebug_xml_set_attr(&child, "name", zpi_val->name->val);
                        sdebug_xml_set_attr(&child, "fullname", zpi_val->name->val);
                    }
                    var_export_xml_node(z_val, &child);
                    sdebug_xml_add_child(node, &child);
                }
                ZEND_HASH_FOREACH_END();
                ZEND_HASH_DEC_APPLY_COUNT(&(tmp_ce->properties_info));

                ht = Z_OBJPROP_P(val);
                ZEND_HASH_INC_APPLY_COUNT(ht);
                ZEND_HASH_FOREACH_KEY_VAL_IND(ht, num, key, z_val)
                {
                    children++;
                    child = sdebug_xml_new_node("property");

                    zend_unmangle_property_name_ex(key, &cls_name, &tmp_prop_name, &tmp_prop_name_len);

                    sdebug_xml_set_attr(&child, "name", tmp_prop_name);
                    sdebug_xml_set_attr(&child, "fullname", tmp_prop_name);

                    var_export_xml_node(z_val, &child);
                    sdebug_xml_add_child(node, &child);
                }
                ZEND_HASH_FOREACH_END();
                ZEND_HASH_DEC_APPLY_COUNT(ht);

                sdebug_xml_set_attr(node, "children", children ? "1" : "0");
                sprintf((char *) tmp_value, "%d", children);
                sdebug_xml_set_attr(node, "numchildren", tmp_value);
            } else {
                sdebug_xml_set_attr(node, "recursive", "1");
            }
            break;
        }
        case IS_RESOURCE: {
            char *type_name;
            sdebug_xml_set_attr(node, "type", "resource");
            type_name = (char *) zend_rsrc_list_get_rsrc_type(Z_RES_P(val));
            sprintf(tmp_value, "resource id='%ld' type='%s'", Z_RES_P(val)->handle, type_name ? type_name : "Unknown");
            sdebug_xml_set_content(node, tmp_value);
            break;
        }
        default:
            sdebug_xml_set_attr(node, "type", "null");
    }
}
