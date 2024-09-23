#ifndef _API_MYFILES_H_
#define _API_MYFILES_H_
#include "api_common.h"
int ApiMyfilesM(uint32_t conn_uuid, std::string &url, std::string &post_data); 
int ApiMyfiles(string &url, string &post_data, string &resp_json);
#endif