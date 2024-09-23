#ifndef _API_LOGIN_H_
#define _API_LOGIN_H_
#include "api_common.h"

 int ApiUserLoginM(u_int32_t conn_uuid, string &post_data); //多线程
 
int ApiUserLogin(string &post_data, string &resp_json);
  
#endif // ! _API_LOGIN_H_
