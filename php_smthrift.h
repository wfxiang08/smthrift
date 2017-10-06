#ifndef PHP_SMTHRIFT_H
#define PHP_SMTHRIFT_H

#include "php.h"
#include "zend_interfaces.h"
#include "zend_exceptions.h"

// 导出thrift c++接口
PHP_FUNCTION(sm_thrift_protocol_write_binary);
PHP_FUNCTION(sm_thrift_protocol_read_binary);

// 声明 smthrift_module_entry，以及指针
extern zend_module_entry smthrift_module_entry;
#define phpext_smthrift_ptr &smthrift_module_entry

#define PHP_SMTHRIFT_VERSION "0.1.0"

#ifdef PHP_WIN32
#	define PHP_SMTHRIFT_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_SMTHRIFT_API __attribute__ ((visibility("default")))
#else
#	define PHP_SMTHRIFT_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

// 定义module的初始化函数&结束函数
PHP_MINIT_FUNCTION (smthrift);

PHP_MSHUTDOWN_FUNCTION (smthrift);

// 定义请求的初始化函数&结束函数
PHP_RINIT_FUNCTION (smthrift);

PHP_RSHUTDOWN_FUNCTION (smthrift);

//声明模块信息函数,即可以在phpinfo看到的信息
PHP_MINFO_FUNCTION (smthrift);

//构造函数，其他接口
PHP_METHOD (smsocket, __construct);

PHP_METHOD (smsocket, pconnect);

PHP_METHOD (smsocket, putBack);


PHP_METHOD (smsocket, read);

PHP_METHOD (smsocket, write);
PHP_METHOD (smsocket, pclose);

PHP_METHOD (smsocket, isStrictWrite);
PHP_METHOD (smsocket, isStrictRead);

#ifdef ZTS
#define SMTHRIFT_G(v) TSRMG(smthrift_globals_id, zend_smthrift_globals *, v)
#else
#define SMTHRIFT_G(v) (smthrift_globals.v)
#endif

typedef struct _smthrift_s {
    char *host;
    unsigned short port;
    zend_bool strict_write;
    zend_bool strict_read;

    long timeoutms;
    php_stream *stream; // 最核心的逻辑
    zend_object zo;
} smthrift_t;

static inline smthrift_t *smthrift_fetch_object(zend_object *obj) {
    return (smthrift_t *) ((char *) obj - XtOffsetOf(smthrift_t, zo));
}

void socket_flush(smthrift_t*s);
size_t socket_write(smthrift_t*s, const char *data, size_t len);
void socket_put_back(smthrift_t*s, const char *data, size_t len);
size_t socket_read(smthrift_t*s, char *data, size_t len);

#define Z_SMTHRIFT_OBJ_P(zv) smthrift_fetch_object(Z_OBJ_P(zv));
#endif	/* PHP_SMTHRIFT_H */
