#include "common.h"
const char* CONFIGFILE = "../config/client.config";

const int MODELOW = 0;
const int MODEHIGH = 1;
const int MAXEPOLLEVENTS = 10000;

//数据相关
const int ONCEMAXREAD = 1024;
//消息类型
const int ERROR = -1;
const int TEXT = 1;
const int IMG  = 2;
const int VIDEO = 3;
const int USERINFO = 4;
const int REGIST = 5;
const int LOGIN = 6;


//客户端状态
const int DISCONNECTED = 0;
const int CONNECTED = 1;