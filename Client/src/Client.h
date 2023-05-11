#pragma once
#include <string>
#include "common.h"
#include "Data.h"
class Data;
class Client
{
public:
    Client(std::string& userName);
    ~Client();
    void run();
    void sendMsg(int type, const char* receiver, const char* content, int contentSize);
    Data* recvMsg();
private:
    void initSocket();
    bool config(std::string configFile);
    
    void creatConnect();

private:
    std::string m_userName;
    int m_socket;
    int m_serverPort;
    std::string m_serverIp;
    int m_state;
};

