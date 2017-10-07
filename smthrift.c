#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "php.h"
#include "ext/standard/info.h"
#include "php_network.h"
#include "php_smthrift.h"

// 类 & handlers
static zend_class_entry *smthrift_ce;
static zend_object_handlers smthrift_object_handlers;

// string $host, int $port, bool $strict_write = true, bool $strict_read=true, int $persitent_key=0
ZEND_BEGIN_ARG_INFO_EX(arginfo___construct, 0, 0, 2)
        ZEND_ARG_INFO(0, host)
        ZEND_ARG_INFO(0, port)
        ZEND_ARG_INFO(0, strict_write)
        ZEND_ARG_INFO(0, strict_read)
        ZEND_ARG_INFO(0, persitent_key)
ZEND_END_ARG_INFO()

// 导出foolsock的方法
const zend_function_entry sm_socket_methods[] = {
    ZEND_ME(smsocket, __construct, arginfo___construct, ZEND_ACC_PUBLIC)
    ZEND_ME(smsocket, pconnect, NULL, ZEND_ACC_PUBLIC)
    ZEND_ME(smsocket, read, NULL, ZEND_ACC_PUBLIC)
    ZEND_ME(smsocket, write, NULL, ZEND_ACC_PUBLIC)
    ZEND_ME(smsocket, pclose, NULL, ZEND_ACC_PUBLIC)
    ZEND_ME(smsocket, putBack, NULL, ZEND_ACC_PUBLIC)
    ZEND_ME(smsocket, isStrictWrite, NULL, ZEND_ACC_PUBLIC)
    ZEND_ME(smsocket, isStrictRead, NULL, ZEND_ACC_PUBLIC)
    PHP_FE_END
};

// 定义导出的方法
zend_function_entry thrift_protocol_functions[] = {
    PHP_FE(sm_thrift_protocol_write_binary, NULL)
    PHP_FE(sm_thrift_protocol_read_binary, NULL)
    {NULL, NULL, NULL}
};


zend_module_entry smthrift_module_entry = {
    STANDARD_MODULE_HEADER, // Module的头部信息
    "smthrift",             // Module的名字
    thrift_protocol_functions,     // 方法列表
    PHP_MINIT(smthrift),
    PHP_MSHUTDOWN(smthrift),
    PHP_RINIT(smthrift),
    PHP_RSHUTDOWN(smthrift),
    PHP_MINFO(smthrift),
    PHP_SMTHRIFT_VERSION,
    STANDARD_MODULE_PROPERTIES
};

// 如果没有这一句，将不会生成有效的扩展so
#ifdef COMPILE_DL_SMTHRIFT
ZEND_GET_MODULE(smthrift)
#endif

static void smsocket_object_free_storage(zend_object *object) {
    smthrift_t *intern = smthrift_fetch_object(object);
    // php_printf("smsocket_object_free_storage\n");

    zend_object_std_dtor(&intern->zo);
}

static zend_object *smsocket_object_new(zend_class_entry *ce) {
    // 如何构造Fool sock对象呢?
    smthrift_t *intern = ecalloc(1, sizeof(smthrift_t) + zend_object_properties_size(ce));

    zend_object_std_init(&intern->zo, ce);
    object_properties_init(&intern->zo, ce);

    // 重载handlers
    intern->zo.handlers = &smthrift_object_handlers;
    // 返回对象
    return &intern->zo;
}

/*{{{ static struct timeval convert_timeoutms_to_ts(long msecs)
 */
static struct timeval convert_timeoutms_to_ts(long msecs) {
    struct timeval tv;
    int secs = 0;

    // ms --> ts
    secs = (int) (msecs / 1000);
    tv.tv_sec = secs;
    tv.tv_usec = (int) (((msecs - (secs * 1000)) * 1000) % 1000000);
    return tv;
}
/*}}}*/

/*{{{ static int get_stream(smthrift_t* f_obj TSRMLS_DC)
 */
static int get_stream(smthrift_t *f_obj TSRMLS_DC) {

    // 如何实现持久操作?
    // Key的定义
    char *hash_key;
    spprintf(&hash_key, 0, "smthrift:%s:%d:%d", f_obj->host, f_obj->port, f_obj->persitent_key);

    // 根据hash_key获取持久化的连接
    switch (php_stream_from_persistent_id(hash_key, &(f_obj->stream) TSRMLS_CC)) {

        case PHP_STREAM_PERSISTENT_SUCCESS:
            // 判断是否出现EOF
            // php_printf("PHP_STREAM_PERSISTENT_SUCCESS\n");
            if (php_stream_eof(f_obj->stream)) {
                php_stream_pclose(f_obj->stream);
                f_obj->stream = NULL;
                // php_printf("c stream1: %p\n", f_obj->stream);
                break;
            }
        case PHP_STREAM_PERSISTENT_FAILURE:
            break;
        default:
            break;
    }

    struct timeval tv = convert_timeoutms_to_ts(f_obj->timeoutms);

    // 创建SocketStream
    if (!f_obj->stream) {
        // php_printf("Open new stream\n");

        // php_stream_sock_open_from_socket
        if (f_obj->host[0] == '/') {
            char *res;
            zend_long reslen;
            php_stream *stream;

            reslen = spprintf(&res, 0, "unix://%s", f_obj->host);

            stream = php_stream_xport_create(res, reslen, REPORT_ERRORS,
                                             STREAM_XPORT_CLIENT | STREAM_XPORT_CONNECT, hash_key, &tv, NULL,
                                             NULL, NULL);

#ifdef DEBUG_LOG
            php_printf("Open new stream for: %s --> %p\n", res, stream);
#endif

            efree(res);
            f_obj->stream = stream;
        } else {

            int socktype = SOCK_STREAM;
            f_obj->stream = php_stream_sock_open_host(f_obj->host, f_obj->port, socktype, &tv, hash_key);
        }

        php_stream_auto_cleanup(f_obj->stream);
        php_stream_set_option(f_obj->stream, PHP_STREAM_OPTION_READ_TIMEOUT, 0, &tv);
        php_stream_set_option(f_obj->stream, PHP_STREAM_OPTION_WRITE_BUFFER, PHP_STREAM_BUFFER_NONE, NULL);
        php_stream_set_chunk_size(f_obj->stream, 8192);

    }
    efree(hash_key);

    if (!f_obj->stream) {
        // 报告失败
#ifdef DEBUG_LOG
        php_printf("Open new stream failed\n");
#endif
        return 0;
    } else {
        // php_printf("Open new stream succeed, stream: %p\n", f_obj->stream);
        return 1;
    }
}
/*}}}*/


/*{{{ public function SmSocket::__construct(string $host, int $port, bool $strict_write = true, bool $strict_read=true, int $persitent_key=0)
 */
PHP_METHOD (smsocket, __construct) {
    smthrift_t *intern;
    long port;
    long persitent_key = 0;
    char *host;
    int host_len;

    // "sl|bb" 如果没有指定参数$strict_write, $strict_read 则对应的参数不会被设置
    zend_bool strict_write = 1;
    zend_bool strict_read = 1;

    // 读取 port, host参数
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl|bbl", &host, &host_len, &port, &strict_write,
                              &strict_read, &persitent_key) == FAILURE) {
        RETURN_FALSE;
    }
    intern = Z_SMTHRIFT_OBJ_P(getThis());
#ifdef DEBUG_LOG
    php_printf("This Conn: %p\n", intern);
#endif

    // 初始化intern
    intern->host = pemalloc((size_t) (host_len + 1), 1);
    memcpy(intern->host, host, host_len);
    intern->host[host_len] = '\0';
    intern->port = (unsigned short) (port);
    intern->persitent_key = persitent_key; // 如果host/port相同，可以通过不同的persitent_key来区分

    intern->strict_read = strict_read;
    intern->strict_write = strict_write;

#ifdef DEBUG_LOG
    php_printf("host: %s, port: %d\n", intern->host, intern->port);
#endif
}
/*}}}*/

PHP_METHOD (smsocket, putBack) {
    php_printf("putBack called\n");
    RETURN_TRUE;
}

PHP_METHOD (smsocket, isStrictWrite) {
    smthrift_t *intern = Z_SMTHRIFT_OBJ_P(getThis());
    if (intern->strict_write) {
        RETURN_TRUE;
    } else {
        RETURN_FALSE;
    }
}

PHP_METHOD (smsocket, isStrictRead) {
    smthrift_t *intern = Z_SMTHRIFT_OBJ_P(getThis());
    if (intern->strict_read) {
        RETURN_TRUE;
    } else {
        RETURN_FALSE;
    }
}

/*{{{ public function SmSocket::pconnect([int $timeoutms])
 */
PHP_METHOD (smsocket, pconnect) {
    smthrift_t *intern;
    long timeoutms = 0;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|l", &timeoutms) == FAILURE) {
        RETURN_FALSE;
    }

    intern = Z_SMTHRIFT_OBJ_P(getThis());
    intern->timeoutms = timeoutms;
#ifdef DEBUG_LOG
    // php_printf("This PConn: %p\n", intern);
#endif

    // 如何获取stream呢?
    int stream_r = get_stream(intern TSRMLS_CC);
    if (stream_r) {
        RETURN_TRUE;
    } else {
        RETURN_FALSE;
    }
}
/*}}}*/

/*{{{ public function SmSocket::write(string $msg)
 */
PHP_METHOD (smsocket, write) {
    smthrift_t *intern;
    char *msg;
    int msg_len;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &msg, &msg_len) == FAILURE) {
        RETURN_FALSE;
    }

    intern = Z_SMTHRIFT_OBJ_P(getThis());
#ifdef DEBUG_LOG
    php_printf("This Write: %p\n", intern);
#endif
    if (intern->stream == NULL) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "Socket Not Connected");
        RETURN_FALSE;
    }

    // 写数据到stream中
    int written = php_stream_write(intern->stream, msg, (size_t) msg_len);
#ifdef DEBUG_LOG
    // php_printf("c write: expected %d, actual: %d, stream: %p\n", msg_len, written, intern->stream);
#endif

    if (written != msg_len) {
        RETURN_FALSE;
    } else {
        RETURN_LONG((long) written);
    }
}
/*}}}*/

/*{{{ public function SmSocket::read(int $size)
 */
PHP_METHOD (smsocket, read) {
    long size;
    smthrift_t *intern;
    char *response_buf;

    // read($size)
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &size) == FAILURE) {
        RETURN_FALSE;
    }

    if (size <= 0) {
        RETURN_TRUE;
    }

    intern = Z_SMTHRIFT_OBJ_P(getThis());
    if (intern->stream == NULL) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "Socket Not Connected");
        RETURN_FALSE;
    }

    // php_printf("c read: stream: %p, length: %d\n", intern->stream, size);

    // php的内存管理?
    response_buf = emalloc(size);
    size_t r = php_stream_read(intern->stream, response_buf, (size_t) size);

    // php_printf("c read: stream: %p, length: %d --> %d\n", intern->stream, size, r);
    if (r <= 0) {
        // 如果是Blocking IO, 应该不用考虑这种情况
        if (errno == EAGAIN || errno == EINPROGRESS) {
            RETURN_TRUE;
        } else {
            RETURN_FALSE;
        }
    }
    RETURN_STRINGL(response_buf, r);
}
/*}}}*/

/*{{{ public function SmSocket::pclose()
 */
PHP_METHOD (smsocket, pclose) {
    smthrift_t *intern = Z_SMTHRIFT_OBJ_P(getThis());

    // 关闭stream
    if (intern->stream != NULL) {
        php_stream_pclose(intern->stream);
        intern->stream = NULL;
    }

    RETURN_TRUE;
}
/*}}}*/


PHP_MINIT_FUNCTION (smthrift) {

    memcpy(&smthrift_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    smthrift_object_handlers.offset = (int) XtOffsetOf(smthrift_t, zo);
    smthrift_object_handlers.clone_obj = NULL;
    smthrift_object_handlers.free_obj = smsocket_object_free_storage;

    zend_class_entry ce;
    INIT_CLASS_ENTRY(ce, "SmSocket", sm_socket_methods);

    // 如果构建smthrift_t对象, 在zo的基础上还多出一截
    smthrift_ce = zend_register_internal_class(&ce TSRMLS_CC);
    smthrift_ce->create_object = smsocket_object_new;

    return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION (smthrift) {
    return SUCCESS;
}

PHP_RINIT_FUNCTION (smthrift) {
    return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION (smthrift) {
    return SUCCESS;
}

PHP_MINFO_FUNCTION (smthrift) {
    php_info_print_table_start();
    php_info_print_table_header(2, "smthrift support", "enabled");
    php_info_print_table_end();
}
