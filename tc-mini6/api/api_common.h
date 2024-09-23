#ifndef _API_COMMON_H_
#define _API_COMMON_H_
#include "cache_pool.h"
#include "db_pool.h"
#include "redis_keys.h"
#include "tc_common.h"
#include "dlog.h"
#include "json/json.h"
#include <string>

#define HTTP_RESPONSE_HTML_MAX 4096
#define HTTP_RESPONSE_HTML                                                     \
    "HTTP/1.1 200 OK\r\n"                                                      \
    "Connection:close\r\n"                                                     \
    "Content-Length:%d\r\n"                                                    \
    "Content-Type:application/json;charset=utf-8\r\n\r\n%s"
    
#define API_LOGIN_MUTIL_THREAD 1
#define API_MYFILES_MUTIL_THREAD 1

using std::string;

 
//获取用户文件个数
int CacheSetCount(CacheConn *cache_conn, string key, int64_t count);
int CacheGetCount(CacheConn *cache_conn, string key, int64_t &count);
int CacheIncrCount(CacheConn *cache_conn, string key);
int CacheDecrCount(CacheConn *cache_conn, string key);
int DBGetUserFilesCountByUsername(CDBConn *db_conn, string user_name, int &count);
template <typename... Args>
std::string FormatString(const std::string& format, Args... args);
//根据file id将文件从fastdfs删除
int RemoveFileFromFastDfs(const char *fileid);
int DBGetShareFilesCount(CDBConn *db_conn, int &count);
//mysql存储的部分信息加载redis缓存
int ApiInit(char *dfs_path_client);
 #endif