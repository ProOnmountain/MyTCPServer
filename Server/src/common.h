#pragma once
const char* CONFIGFILE = "../config/server.config";



const int MODETHREADS = 0;
const int MODETHREADPOOL = 1;
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

//套接字状态
const int FREE = 1;
const int DEALING = 2;
