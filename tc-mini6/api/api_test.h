/**
 * 这里存储是为了测试http + json的性能，不涉及redis、mysql操作
 */
#ifndef _API_TEST_H_
#define _API_TEST_H_
#include "api_common.h"


//多线程操作
void ApiTestM(u_int32_t conn_uuid, string &post_data);
//单线程操作
int ApiTestS(string &post_data, string &resp_json );

 
#endif // ! _API_TEST_H_
