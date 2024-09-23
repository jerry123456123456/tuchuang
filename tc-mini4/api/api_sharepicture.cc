#include "api_common.h"
#include "api_sharepicture.h" 

int decodeSharePictureJson( string &str_json, string &user_name, string &token,
                           string &md5, string &filename) {
    bool res;
    Json::Value root;
    Json::Reader jsonReader;
    res = jsonReader.parse(str_json, root);
    if (!res) {
        LogError("parse SharePictureJson json failed ");
        return -1;
    }

    if (root["token"].isNull()) {
        LogError("token null\n");
        return -1;
    }
    token = root["token"].asString();

    if (root["user"].isNull()) {
        LogError("user null\n");
        return -1;
    }
    user_name = root["user"].asString();

    if (root["md5"].isNull()) {
        LogError("md5 null\n");
        return -1;
    }
    md5 = root["md5"].asString();

    if (root["filename"].isNull()) {
        LogError("filename null\n");
        return -1;
    }
    filename = root["filename"].asString();

    return 0;
}

void encodeSharePictureJson(int ret, string urlmd5, string &str_json) {
    Json::Value root;
    root["code"] = ret;
    if (HTTP_RESP_OK == ret)
        root["urlmd5"] = urlmd5;
    Json::FastWriter writer;
    str_json = writer.write(root);
}

//分享图片
void handleSharePicture(const char *user, const char *filemd5,
                       const char *file_name, string &str_json) {
    //数据库操作
    // 获取数据库连接
    CDBManager *db_manager = CDBManager::getInstance();
    CDBConn *db_conn = db_manager->GetDBConn("tuchuang_master");
    AUTO_REL_DBCONN(db_manager, db_conn);

    //操作数据库
    string urlmd5 = RandomString(32);  // uuid生成唯一值，再转成base64
    //分享时间
    //获取当前时间
    time_t now = time(NULL);
    char create_time[TIME_STRING_LEN];
    strftime(create_time, TIME_STRING_LEN - 1, "%Y-%m-%d %H:%M:%S", localtime(&now));

    LogInfo("urlmd5:{}", urlmd5);
    char sql_cmd[SQL_MAX_LEN] = {0};
    char key[5] = {0};
    sprintf(sql_cmd,
            "insert into share_picture_list (user, filemd5, file_name, urlmd5, "
            "`key`, pv, create_time) values ('%s', '%s', '%s', '%s', '%s', %d, "
            "'%s')",   user, filemd5, file_name, urlmd5.c_str(), key, 0, create_time);
    LogInfo("执行: {}", sql_cmd);
    if (!db_conn->ExecuteCreate(sql_cmd)) {
        LogError("{} 操作失败", sql_cmd);
        encodeSharePictureJson(HTTP_RESP_FAIL, urlmd5, str_json);
    } else {
        encodeSharePictureJson(HTTP_RESP_OK, urlmd5, str_json);
    }        
}

int decodeBrowsePictureJson(string &str_json, string &urlmd5) {
    bool res;
    Json::Value root;
    Json::Reader jsonReader;
    res = jsonReader.parse(str_json, root);
    if (!res) {
        LogError("parse reg json failed ");
        return -1;
    }

    if (root["urlmd5"].isNull()) {
        LogError("urlmd5 null\n");
        return -1;
    }
    urlmd5 = root["urlmd5"].asString();

    return 0;
}

void encodeBrowselPictureJson(int ret, int pv, string url, string user,
                             string time, string &str_json) {
    Json::Value root;
    root["code"] = ret;
    if (ret == 0) {
        root["pv"] = pv;
        root["url"] = url;
        root["user"] = user;
        root["time"] = time;
    }
    Json::FastWriter writer;
    str_json = writer.write(root);
}

void handleBrowsePicture(const char *urlmd5, string &str_json) {
    int ret = 0;
    char sql_cmd[SQL_MAX_LEN] = {0};
    string picture_url;
    string file_name;
    string user;
    string filemd5;
    string create_time;
    int pv = 0;

    //查询 share_picture_list， file_info的url
    CDBManager *db_manager = CDBManager::getInstance();
    CDBConn *db_conn = db_manager->GetDBConn("tuchuang_slave");
    AUTO_REL_DBCONN(db_manager, db_conn);
    CResultSet *result_set = NULL;
     LogInfo("urlmd5: {}", urlmd5);
    // 1. 先从分享图片列表查询到文件信息
    sprintf(sql_cmd,
            "select user, filemd5, file_name, pv, create_time from "
            "share_picture_list where urlmd5 = '%s'", urlmd5);
    LogDebug("执行: {}", sql_cmd);
    result_set = db_conn->ExecuteQuery(sql_cmd);
    if (result_set && result_set->Next()) {
        user = result_set->GetString("user");
        filemd5 = result_set->GetString("filemd5");
        file_name = result_set->GetString("file_name");
        pv = result_set->GetInt("pv");
        create_time = result_set->GetString("create_time");
        delete result_set;
    } else {
        if (result_set)
            delete result_set;
        ret = -1;
        goto END;
    }     
    //查询file_info url
    // 2. 通过文件的MD5查找对应的url地址
    sprintf(sql_cmd, "select url from file_info where md5 ='%s'",
            filemd5.c_str());
    LogInfo("执行: {}", sql_cmd);
    result_set = db_conn->ExecuteQuery(sql_cmd);
    if (result_set && result_set->Next()) {
        picture_url = result_set->GetString("url");
        delete result_set;
    } else {
        if (result_set)
            delete result_set;
        ret = -1;
        goto END;
    }
    LogInfo("url: {}", picture_url);  

    // 3. 更新浏览次数， 可以考虑保存到redis，减少数据库查询的压力 修改、创建是比较花时间
    pv += 1; //浏览计数增加
    sprintf(sql_cmd,
            "update share_picture_list set pv = %d where urlmd5 = '%s'", pv,
            urlmd5);
    LogDebug("执行: {}", sql_cmd);
    if (!db_conn->ExecuteUpdate(sql_cmd)) {
        LogError("{} 操作失败", sql_cmd);
        ret = -1;
        goto END;
    }
    ret = 0;
END:
     // 4. 返回urlmd5 和提取码key
    if (ret == 0) {
        encodeBrowselPictureJson(HTTP_RESP_OK, pv, picture_url, user,
                                 create_time, str_json);
    } else {
        encodeBrowselPictureJson(HTTP_RESP_FAIL, pv, picture_url, user,
                                 create_time, str_json);
    }
}
//解析的json包
int decodePictureListJson(string &str_json, string &user_name, string &token,
                          int &start, int &count) {
    bool res;
    Json::Value root;
    Json::Reader jsonReader;
    res = jsonReader.parse(str_json, root);
    if (!res) {
        LogError("parse reg json failed ");
        return -1;
    }

    if (root["token"].isNull()) {
        LogError("token null\n");
        return -1;
    }
    token = root["token"].asString();

    if (root["user"].isNull()) {
        LogError("user null\n");
        return -1;
    }
    user_name = root["user"].asString();

    if (root["start"].isNull()) {
        LogError("start null\n");
        return -1;
    }
    start = root["start"].asInt();

    if (root["count"].isNull()) {
        LogError("count null\n");
        return -1;
    }
    count = root["count"].asInt();

    return 0;
}

//获取用户共享图片格式
int getSharePicturesCount(CDBConn *db_conn, string user_name, int &count) {
    count = 0;
    int ret = 0;
    // 先查看用户是否存在
    string str_sql;

    str_sql = FormatString("select count(*) from share_picture_list where user='%s'",
                     user_name.c_str());
    LogInfo("执行: {}", str_sql);
    CResultSet *result_set = db_conn->ExecuteQuery(str_sql.c_str());
    if (result_set && result_set->Next()) {
        // 存在在返回
        count = result_set->GetInt("count(*)");
        LogInfo("count: {}", count);
        ret = 0;
        delete result_set;
    } else if (!result_set) { // 操作失败
        LogError("{} 操作失败", str_sql);
        ret = -1;
    } else {
        // 没有记录则初始化记录数量为0
        ret = 0;
        LogInfo("没有记录: count: {}", count);
    }
    return ret;
}

//获取共享文件列表
//获取用户文件信息 127.0.0.1:80/sharepicture&cmd=normal
void handleGetSharePicturesList(const char *user, int start, int count,
                                string &str_json) { 
//查询share_picture_list
//查询url  file_info
    int ret = 0;
    char sql_cmd[SQL_MAX_LEN] = {0};
    CResultSet *result_set = NULL;
    CDBManager *db_manager = CDBManager::getInstance();
    CDBConn *db_conn = db_manager->GetDBConn("tuchuang_slave");
    AUTO_REL_DBCONN(db_manager, db_conn);
    Json::Value root;
    
    int file_count = 0;
    //获取我的图片总分享数量
    int total = 0;
    string temp_user = user;
    ret = getSharePicturesCount(db_conn, temp_user, total);
    if (ret < 0) {
        LogError("getSharePicturesCount failed");
        ret = -1;
        goto END;
    }
    if (total == 0) {
        LogInfo("getSharePicturesCount count = 0");
        ret = 0;
        goto END;
    }
    //有记录 继续查询读取数据
    // sql语句  不需要查询 文件资源的http url
    sprintf(
        sql_cmd,
        "select share_picture_list.user, share_picture_list.filemd5, share_picture_list.file_name,share_picture_list.urlmd5, share_picture_list.pv, \
        share_picture_list.create_time, file_info.size from file_info, share_picture_list where share_picture_list.user = '%s' and  \
        file_info.md5 = share_picture_list.filemd5 limit %d, %d", user, start, count);

        LogInfo("执行: {}", sql_cmd);
    result_set = db_conn->ExecuteQuery(sql_cmd);
    if (result_set) {
        // 遍历所有的内容
        // 获取大小
        Json::Value files;
        while (result_set->Next()) {
            Json::Value file;
            file["user"] = result_set->GetString("user");
            file["filemd5"] = result_set->GetString("filemd5");
            file["file_name"] = result_set->GetString("file_name");
            file["urlmd5"] = result_set->GetString("urlmd5");
            file["pv"] = result_set->GetInt("pv");
            file["create_time"] = result_set->GetString("create_time");
            file["size"] = result_set->GetInt("size");
            files[file_count] = file;
            file_count++;
        }
        if (file_count > 0)
            root["files"] = files;

        ret = 0;
        delete result_set;
    } else {
        ret = -1;
    }     
END:
    //json最后的序列化
    if(ret != 0) {
       root["code"] = HTTP_RESP_FAIL;
    } else {
        root["code"] = 0;
        root["count"] = file_count; // 10
        root["total"] = total;  //20 
    }
    str_json = root.toStyledString();
    LogInfo("str_json: {}",str_json);
}


int decodeCancelPictureJson(string &str_json, string &user_name,
                            string &urlmd5) {
    bool res;
    Json::Value root;
    Json::Reader jsonReader;
    res = jsonReader.parse(str_json, root);
    if (!res) {
        LogError("parse reg json failed ");
        return -1;
    }

    if (root["user"].isNull()) {
        LogError("user null\n");
        return -1;
    }
    user_name = root["user"].asString();

    if (root["urlmd5"].isNull()) {
        LogError("urlmd5 null\n");
        return -1;
    }
    urlmd5 = root["urlmd5"].asString();

    return 0;
}

int encodeCancelPictureJson(int ret, string &str_json) {
    Json::Value root;
    root["code"] = ret;
    Json::FastWriter writer;
    str_json = writer.write(root);
    return 0;
}
//取消分享文件
void handleCancelSharePicture(const char *user, const char *urlmd5,
                              string &str_json) {
    char sql_cmd[SQL_MAX_LEN] = {0};
   
    CDBManager *db_manager = CDBManager::getInstance();
    CDBConn *db_conn = db_manager->GetDBConn("tuchuang_master");
    AUTO_REL_DBCONN(db_manager, db_conn);
    LogWarn("into");
 
    // 获取文件md5
    LogInfo("urlmd5: {}", urlmd5);
    //删除在共享图片列表的数据
    sprintf(sql_cmd, "delete from share_picture_list where user = '%s' and urlmd5 = '%s'",  user, urlmd5);
    LogInfo("执行: {}", sql_cmd);
    if (!db_conn->ExecutePassQuery(sql_cmd)) {
        LogError("{} 操作失败", sql_cmd);
        encodeCancelPictureJson(HTTP_RESP_FAIL, str_json);
    } else {
        encodeCancelPictureJson(HTTP_RESP_OK, str_json);
    }
}

int ApiSharepicture(string &url, string &post_data, string &resp_json) {
    char cmd[20];
    string user_name; //用户名
    string md5;       //文件md5码
    string urlmd5;
    string filename; //文件名字
    string token;
    int ret = 0;
    int start = 0;
    int count = 0;
    //解析命令
    QueryParseKeyValue(url.c_str(), "cmd", cmd, NULL);
    LogInfo("cmd = {}", cmd);

    if(strcmp(cmd, "share") == 0) {
        //解析json
        if(decodeSharePictureJson(post_data, user_name, token, md5, filename) < 0) {
            encodeSharePictureJson(HTTP_RESP_FAIL, "", resp_json);
            return -1;
        }
        //token校验
        //校验token
        ret = VerifyToken(user_name, token); 
        if(ret < 0) {
            encodeSharePictureJson(HTTP_RESP_TOKEN_ERR, "", resp_json);
            return -1;
        }
        //创建分享连接 用户名 文件md5 文件名 获取返回结果resp_json
        handleSharePicture(user_name.c_str(), md5.c_str(), filename.c_str(), resp_json);
    } else if(strcmp(cmd, "browse") == 0) {
        //解析json
        if(decodeBrowsePictureJson(post_data, urlmd5) < 0) {
            encodeSharePictureJson(HTTP_RESP_FAIL, "", resp_json);
            return -1;
        }
        //urlmd5  获取返回结果resp_json
        handleBrowsePicture(urlmd5.c_str(), resp_json);
    } else if (strcmp(cmd, "normal") == 0) {
        //解析json
        if(decodePictureListJson(post_data, user_name, token, start, count) < 0) {
            encodeSharePictureJson(HTTP_RESP_FAIL, "", resp_json);
            return -1;
        }
        //token校验
        //校验token
        ret = VerifyToken(user_name, token); 
        if(ret < 0) {
            encodeSharePictureJson(HTTP_RESP_TOKEN_ERR, "", resp_json);
            return -1;
        }
        //读取数据库 封装json
        handleGetSharePicturesList(user_name.c_str(), start, count, resp_json);
    } else if (strcmp(cmd, "cancel") == 0) {
        //解析json user , urlmd5
        if(decodeCancelPictureJson(post_data, user_name, urlmd5) < 0) {
            encodeCancelPictureJson(HTTP_RESP_FAIL,  resp_json);
            return -1;
        }
        //token暂时不校验

        //取消分享
        handleCancelSharePicture(user_name.c_str(), urlmd5.c_str(), resp_json);

    } else {
        LogError("unknow cmd: {}", cmd);
    }


    return 0;
}
