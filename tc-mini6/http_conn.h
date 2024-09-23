/*
 * http_conn.h
 *
 *  Created on: 2013-9-29
 *      Author: ziteng
 */

#ifndef __HTTP_CONN_H__
#define __HTTP_CONN_H__

#include "http_parser_wrapper.h"
#include "netlib.h"
#include "util.h"
#include <mutex>
#include "tc_thread_pool.h" 
extern ThreadPool g_thread_pool; 
#define READ_BUF_SIZE 2048

#define HTTP_RESPONSE_HTML                                                     \
    "HTTP/1.1 200 OK\r\n"                                                      \
    "Connection:close\r\n"                                                     \
    "Content-Length:%d\r\n"                                                    \
    "Content-Type:application/json;charset=utf-8\r\n\r\n%s"

#define HTTP_RESPONSE_HTML_MAX 4096

enum {
    CONN_STATE_IDLE,
    CONN_STATE_CONNECTED,
    CONN_STATE_OPEN,
    CONN_STATE_CLOSED,
};

 typedef struct {
    uint32_t conn_uuid; // 用于查找connection
    string resp_data;   // 要回发的数据
} ResponsePdu_t;
// 继承引用计数的基类，当引用计数减到0时析构对象
class CHttpConn : public CRefObject {
public:
    //构造函数
    CHttpConn();
    //析构函数
    virtual ~CHttpConn();
    // 获取当前自定义的唯一handle
    uint32_t GetConnHandle() { return conn_handle_; }
    // 返回对端ip
    char *GetPeerIP() { return (char *)peer_ip_.c_str(); }
    //发送数据，最终调用socket的send
    int Send(void *data, int len);
    //关闭连接，最终调用socket的close
    void Close();
    // accept收到新连接fd，触发OnConnect的调用，并把socket fd传递进来
    void OnConnect(net_handle_t socket_handle);
    // 可读事件触发回调OnRead
    void OnRead();
    // 可写事件触发回调OnWrite
    void OnWrite();
    // 关闭事件触发OnClose
    void OnClose();
    // 数据发送完毕回调OnWriteComlete，自己业务处理的
    void OnWriteComlete();
    static void AddResponseData(uint32_t conn_uuid, string &resp_data); // 工作线程调用
    static void SendResponseDataList();             // 主线程调用
private:      
    // 账号注册处理
    void _HandleRegisterRequest(string &url, string &post_data);
    // 账号登陆处理
    void _HandleLoginRequest(string &url, string &post_data);
    void _HandleLoginMRequest(string &url, string &post_data);
    //获取我的文件列表
    void _HandleMyfilesRequest(string &url, string &post_data);
    void _HandleMyfilesMRequest(string &url, string &post_data);
    //秒传
    void _HandleMd5Request(string &url, string &post_data);
    //文件上传
    void  _HandleUploadRequest(string &url, string &post_data);
    //分享图片相关操作：分享、浏览、取消
    void _HandleSharepictureRequest(string &url, string &post_data);

    //处理文件请求 比如删除等
    void _HandleDealfileRequest(string &url, string &post_data);
    // 
    void _HandleDealsharefileRequest(string &url, string &post_data);
    void _HandleSharefilesRequest(string &url, string &post_data);

    void _HandleTestMRequest(string &url, string &post_data);
    void _HandleTestSRequest(string &url, string &post_data);
 protected:
    //socket fd
    net_handle_t socket_handle_;  
    // 业务自定义唯一标识
    uint32_t conn_handle_;
    //当前socket写缓存是否有空间，=ture当前socket写缓存已满，=false当前socket写缓存有空间可以写
    bool busy_;
    //连接状态
    uint32_t state_;
    //保存对端ip
    std::string peer_ip_;
    //保存对端端口
    uint16_t peer_port_;
    //缓存从socket读取的数据
    CSimpleBuffer in_buf_;
    //缓存还没有发送出去的数据
    CSimpleBuffer out_buf_;

    // http解析
    CHttpParserWrapper http_parser_;

    static std::mutex s_resp_mutex;
    static std::list<ResponsePdu_t *> s_response_pdu_list; // 主线程发送回复消息
};



#endif /* IMCONN_H_ */
