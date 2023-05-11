#include "Client.h"

#include <sys/socket.h>
#include <iostream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <fstream>
#include <sstream>
#include <unistd.h>

Client::Client(std::string& userName):
    m_userName(userName),
    m_socket(-1),
    m_serverPort(-1),
    m_state(DISCONNECTED){
    initSocket();
}

Client::~Client(){
}

void Client::run(){
    if(config(CONFIGFILE) == false){
        std::cerr << "配置失败，请检查配置文件\n";
        exit(-1);
    }
    sockaddr_in serverAddr = {0};
    serverAddr.sin_port = htons(m_serverPort);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(m_serverIp.c_str());
    if(connect(m_socket, (sockaddr*)&serverAddr, sizeof(sockaddr)) == -1){
        m_state = DISCONNECTED;
        std::cerr << "连接服务器失败，" << strerror(errno) << std::endl;
        exit(-1);
    }
    creatConnect();
}

void Client::sendMsg(int type, const char *receiver, const char *content, int contentSize){
    if(m_socket == -1){
        std::cerr << "套接字错误\n";
        exit(-1);
    }
    Data data;
    data.m_header.m_dataType = type;
    strcpy(data.m_header.m_receiver, receiver);
    strcpy(data.m_header.m_sender, m_userName.c_str());
    data.m_header.m_bodySize = contentSize;
    data.m_body = new char[contentSize];
    memcpy(data.m_body, content, contentSize);
    int writeLen = write(m_socket, &data.m_header, sizeof(Header));
    if(writeLen != sizeof(Header)){
        std::cerr << "数据头部发送失败\n";
        return;
    }
    int sendLen = 0;
    while(sendLen < data.m_header.m_bodySize){
        writeLen = write(m_socket, data.m_body, data.m_header.m_bodySize);
        if(writeLen != -1){
            sendLen += writeLen;
        }
        else{
            std::cerr << "无法向服务器写入数据，" << strerror(errno) << std::endl;
            if(errno == EPIPE){
                shutdown(m_socket, SHUT_RDWR);
                close(m_socket);
                m_socket = -1;
            }
            return;
        }
    }
    std::cout << "数据发送成功\n";
}

void Client::initSocket(){
    m_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(m_socket == -1){
        std::cerr << "套接字创建失败\n";
        exit(-1);
    }
}

bool Client::config(std::string configFile){
    std::fstream file(configFile, std::ios_base::in);
    if(!file){
        std::cerr << "配置文件打开错误：" << strerror(errno) << std::endl;
        exit(1);
    }
    std::string line;
    while(file >> line)
    {
        if(!line.empty()){
            std::stringstream ss(line);
            std::string key, value;
            std::getline(ss, key, ':');
            ss >> value;
            if(key == "server_ip"){
                if(value.empty()){
                    std::cout << "服务器IP为空，请检查配置文件\n";
                    exit(0);
                }
                else{
                    m_serverIp = value;
                }
                std::cout << "服务器IP配置成功\n";
            }
            else if(key == "server_port"){
                int n = atoi(value.c_str());
                if(n >= 6000){
                    m_serverPort = n;
                }
                else{
                    std::cout << "服务器端口号小于6000，请检查配置文件\n";
                    exit(0);
                }
                std::cout << "端口配置成功\n";
            }
        }
    }
    file.close();
    if((!m_serverIp.empty()) && (m_serverPort != -1))
    {
        return true;
    }
    return false;
}

Data *Client::recvMsg(){
    if(m_socket == -1){
        std::cerr << "无效的套接字\n";
        return nullptr;
    }
    Header header;
    if(read(m_socket, &header, sizeof(Header)) == -1){
        std::cerr << "获取数据头部出错\n";
        return nullptr;
    }
    Data *data = new Data;
    data->m_header = header;
    data->m_body = new char[header.m_bodySize];
    int recvLen = 0;
    while(recvLen < header.m_bodySize){
        int readLen = read(m_socket, data->m_body + recvLen, header.m_bodySize);
        if(readLen != -1){
            recvLen += readLen;
        }
        else{
            std::cerr << "无法读取数据，" << strerror(errno) << std::endl;
            delete data;
            data = nullptr;
            if(errno == EPIPE){
                shutdown(m_socket, SHUT_RDWR);
                close(m_socket);
                m_socket = -1;
            }
            delete data;
            return nullptr;
        }
    }
    return data;
}

void Client::creatConnect(){
    sendMsg(USERINFO, "Server", nullptr, 0);
    Data* data = recvMsg();
    if(data == nullptr){
        std::cerr << "连接建立失败\n";
        m_state = DISCONNECTED;
        return;
    }
    if(data->m_header.m_dataType == USERINFO){
        std::cout << "成功连接服务器\n";
        m_state = CONNECTED;
    }
}
