#include "api_md5.h"
#include "api_common.h"
enum Md5State {
    Md5Ok = 0,
    Md5Failed = 1,
    Md5TokenFaild = 4,
    Md5FileExit = 5,
};

 
 int decodeMd5Json(string &str_json, string &user_name, string &token,
                  string &md5, string &filename)  {
    bool res;
    Json::Value root;
    Json::Reader jsonReader;
    res = jsonReader.parse(str_json, root);
    if (!res) {
        LogError("parse md5 json failed ");
        return -1;
    }

    if (root["user"].isNull()) {
        LogError("user null");
        return -1;
    }
    user_name = root["user"].asString();

    if (root["token"].isNull()) {
        LogError("token null");
        return -1;
    }
    token = root["token"].asString();

    if (root["md5"].isNull()) {
        LogError("md5 null");
        return -1;
    }
    md5 = root["md5"].asString();

    if (root["filename"].isNull()) {
        LogError("filename null");
        return -1;
    }
    filename = root["filename"].asString();

    return 0;
}

void encodeMd5Json(int ret, string &str_json) {
    Json::Value root;
    root["code"] = ret;
    Json::FastWriter writer;
    str_json = writer.write(root);    
}
void handleDealMd5(const char *user, const char *md5, const char *filename,
                   string &str_json) 
{
    //去file_info查询md5是否存在
    //" select count from file_info where md5 = '%s'", md5 
    //msyql操作的问题， mysql连接池
    int ret = 0;
    char sql_cmd[SQL_MAX_LEN] = {0};
    CDBManager *db_manager = CDBManager::getInstance();
    CDBConn *db_conn = db_manager->GetDBConn("tuchuang_master");
    AUTO_REL_DBCONN(db_manager, db_conn);

    // 1.先查询file_info表里有没有该文件的存储信息
    //  sql 语句，获取此md5值文件的文件计数器 count
    sprintf(sql_cmd, "select count from file_info where md5 = '%s'", md5); 
    //count是对应md5对应文件被引用的次数
    LogInfo("执行: {}", sql_cmd);
    int file_ref_count = 0;     //文件引用计数
    ret = GetResultOneCount(db_conn, sql_cmd, file_ref_count);
    if(ret == 0) {  //查询有结果，有行纪录 md5存在
        //2. 查看此用户是否已经有此文件，如果存在说明此文件已上传，无需再上传
        sprintf(sql_cmd, "select * from user_file_list where user = '%s' and md5 = '%s' and file_name = '%s'", user, md5, filename);
        LogInfo("执行: {}", sql_cmd);
        //返回值： 1: 表示已经存储了，有这个文件记录
        ret = CheckwhetherHaveRecord(db_conn, sql_cmd); // 检测个人是否有记录
        if(ret == 1) {
            LogWarn("user: {}->  filename: {}, md5: {}已存在", user, filename, md5);
            encodeMd5Json(Md5FileExit, str_json);
        } else if(ret == 0) { //重点处理这个逻辑
            // 更新file_info count
            sprintf(sql_cmd, "update file_info set count = %d where md5 = '%s'",
                file_ref_count + 1, md5);
            LogInfo("执行: {}", sql_cmd);
            if (!db_conn->ExecutePassQuery(sql_cmd)) {
                LogError("{} 操作失败", sql_cmd);
                encodeMd5Json(Md5Failed, str_json);
                return;
            }
            // 添加用户文件列表记录
            //当前时间戳
            struct timeval tv;
            struct tm *ptm;
            char time_str[128];

            //使用函数gettimeofday()函数来得到时间。它的精度可以达到微妙
            gettimeofday(&tv, NULL);
            ptm = localtime(&tv.tv_sec); //把从1970-1-1零点零分到当前时间系统所偏移的秒数时间转换为本地时间
            // strftime()
            // 函数根据区域设置格式化本地时间/日期，函数的功能将时间格式化，或者说格式化一个时间字符串
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", ptm);

            //user file list , insert
            sprintf(sql_cmd,   "insert into user_file_list(user, md5, create_time, file_name, "
                "shared_status, pv) values ('%s', '%s', '%s', '%s', %d, %d)",
                user, md5, time_str, filename, 0, 0);
             LogInfo("执行: {}", sql_cmd);
            if (!db_conn->ExecuteCreate(sql_cmd)) {
                LogError("{} 操作失败", sql_cmd);
                // 恢复引用计数
                sprintf(sql_cmd, "update file_info set count = %d where md5 = '%s'", file_ref_count, md5);
                LogInfo("执行: {}", sql_cmd);
                if (!db_conn->ExecutePassQuery(sql_cmd)) {
                    LogError("{} 操作失败", sql_cmd);
                }
                encodeMd5Json(Md5Failed, str_json);
            }
            encodeMd5Json(Md5Ok, str_json);
        } else {
            LogInfo("CheckwhetherHaveRecord失败: {}", ret);
            encodeMd5Json(Md5Failed, str_json);
        }
    } else {
         LogInfo("秒传失败");
         encodeMd5Json(Md5Failed, str_json);
    }

}
int ApiMd5(string &url, string &post_data, string &resp_json) {

    // 解析json
    string user;
    string md5;
    string token;
    string filename;
    int ret = 0;

    if(decodeMd5Json(post_data, user, token, md5, filename) != 0) {
        LogError("decodeMd5Json() err");
        //封装code =  Md5Failed
        encodeMd5Json(Md5Failed, resp_json);
        return 0;
    }
    //校验token
    ret = VerifyToken(user, token); // util_cgi.h
    if (ret == 0) {
        //秒传业务的处理
        // in: user  md5 filename,  out:resp_json
        handleDealMd5(user.c_str(), md5.c_str(), filename.c_str(), resp_json);
        return 0;
    } else {
        // 校验失败
        encodeMd5Json(Md5TokenFaild, resp_json);
        return 0; 
    }
}