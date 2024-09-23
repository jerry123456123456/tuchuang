
#include "api_test.h"
#include "http_conn.h"
//多线程操作
void ApiTestM(u_int32_t conn_uuid, string &post_data) {
    string resp_json = "{\"code\" : 0}";
    char *str_content = new char[HTTP_RESPONSE_HTML_MAX];
    uint32_t ulen = resp_json.length();
    snprintf(str_content, HTTP_RESPONSE_HTML_MAX, HTTP_RESPONSE_HTML, ulen,
             resp_json.c_str());
    resp_json = str_content;
    CHttpConn::AddResponseData(conn_uuid, resp_json);
    delete str_content;
}
//单线程操作
int ApiTestS(string &post_data, string &resp_json ) {
    //不管发什么信息过来，直接回复
    resp_json = "{\"code\" : 0}";
    return 0;
}