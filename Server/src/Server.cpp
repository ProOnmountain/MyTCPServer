#include "Server.h"
#include "common.h"
#include "Data.h"
#include "ThreadPool.h"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cerrno>
#include <string.h>
#include <fstream>
#include <sstream>
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <thread>
#include <functional>
#include <signal.h>

Server::Server() : 
    m_ip(""), 
    m_port(-1),
    m_mutex_clientSockets(new std::mutex),
    m_serverSocket(-1){
    m_clientSockets.clear();
    if(config(CONFIGFILE) == true){
        initSocket();
    }
}

Server::~Server(){
    close(m_serverSocket);
    m_serverSocket = -1;
    std::cout << "服务器关闭\n";
}

void Server::run(){
    if(m_serverSocket == -1)
    {
        std::cerr << "无效的套接字\n";
        exit(1);
    }
    if(listen(m_serverSocket, 0) < 0)
    {
        std::cerr << "服务器监听失败: " << strerror(errno) << std::endl;
        close(m_serverSocket);
        m_serverSocket = -1;
        exit(1);
    }
    std::cout << "启动监听, 端口：" << m_port << std::endl;
    if(m_mode == MODETHREADS)
    {
        runModeThreadPool();
    }
    else if(m_mode == MODETHREADPOOL)
    {
        runModeThreads();
    }
    else
    {
        std::cerr << "错误的运行模式\n";
    }
}

bool Server::config(std::string configFile){
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
                    m_ip = "127.0.0.1";
                    std::cout << "IP为空，设置为默认IP\n";
                }
                else{
                    m_ip = value;
                }
                std::cout << "IP配置成功\n";
            }
            else if(key == "server_port"){
                int n = atoi(value.c_str());
                if(n >= 6000){
                    m_port = n;
                }
                else{
                    m_port = 9000;
                    std::cout << "端口号小于6000，设置为默认端口（9000）\n";
                }
                std::cout << "端口配置成功\n";
            }
            else if(key == "server_mode"){
                int n = atoi(value.c_str());
                if(n == MODETHREADS || n == MODETHREADPOOL){
                    m_mode = n;
                }
                else{
                    m_mode = MODETHREADS;
                }
            }
            else if(key == "server_thread_size"){
                m_threadNum = atoi(value.c_str());
            }
        }
    }
    file.close();
    if((!m_ip.empty()) && (m_port != -1))
    {
        return true;
    }
    return false;
}

void Server::initSocket(){
    m_serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(m_serverSocket == -1){
        std::cerr << "服务器套接字创建失败\n";
        exit(1);
    }
    else{

        sockaddr_in serverAddr = {0};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(m_port);
        if(m_ip.empty())
        {
            serverAddr.sin_addr.s_addr = inet_addr(INADDR_ANY);
        }
        else if(inet_aton(m_ip.c_str(), &serverAddr.sin_addr) == 0){
            std::cerr << "无效的ip地址\n";
            m_serverSocket = -1;
            exit(1);
        }
        if(bind(m_serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
        {
            std::cerr << "绑定套接字和ip、端口失败：" << strerror(errno) << std::endl;
            m_serverSocket = -1;
            exit(1);
        }
        std::cout << "服务器套接字创建成功\n";
    }
}

void Server::runModeThreads(){
    if(m_serverSocket == -1){
        return;
    }
    std::cout << "阻塞模式\n";
    while(1){
        sockaddr_in clientAddr = {0};
        socklen_t n = sizeof(clientAddr);
        int client_scoket = accept(m_serverSocket, (sockaddr*)&clientAddr, &n);
        if(client_scoket > 0){
            if(creatConnect(client_scoket) == false){
                return;
            }
            //处理客户端数据数据
            auto f = [=](){
                while(1){
                    int a;
                    // std::cin >> a;
                    Data* data = readData(client_scoket);
                    if(data != nullptr){
                        std::cout << "消息内容：" << data->m_body << std::endl;
                        dealData(data);
                    }
                    else{
                        break;
                    }
                    delete data;
                }
            };
            std::thread *dealThread = new std::thread(f);
            dealThread->detach();
        }
    }
}

void Server::runModeThreadPool(){
    if(m_serverSocket == -1){
        return;
    }
    std::cout << "epoll模式\n";
    //创建线程池
    ThreadPool pool(m_threadNum);
    //创建epoll
    int epfd = epoll_create1(EPOLL_CLOEXEC);
    if(epfd == -1){
        std::cerr << "epoll创建失败\n";
        close(m_serverSocket);
        close(epfd);
        m_serverSocket = -1;
        exit(1);
    }

    //将监听套接字加入epoll
    int flags = fcntl(m_serverSocket, F_GETFL, 0);
    fcntl(m_serverSocket, F_SETFL, flags | O_NONBLOCK);//将监听套接字设置为非阻塞，避免epoll一直等待在监听套接字上
    epoll_event event;
    event.data.fd = m_serverSocket;
    event.events = EPOLLIN | EPOLLET;
    if(epoll_ctl(epfd, EPOLL_CTL_ADD, m_serverSocket, &event) == -1){
        std::cerr << "epoll添加监听套接字失败\n";
        close(m_serverSocket);
        close(epfd);
        m_serverSocket = -1;
        exit(1);
    }
    std::vector<epoll_event> events(MAXEPOLLEVENTS);
    while(1){
        // std::cout << "epoll wait\n";
        int fdNum = epoll_wait(epfd, events.data(), MAXEPOLLEVENTS, -1);
        if(fdNum == -1){
            std::cerr << "epoll等待错误\n";
            continue;
        }
        for(int i = 0; i < fdNum; ++i){
            if(events[i].data.fd == m_serverSocket){
                sockaddr_in clientAddr = {0};
                socklen_t n = sizeof(clientAddr);
                int client_scoket = accept(m_serverSocket, (sockaddr*)&clientAddr, &n);
                if(client_scoket > 0){
                    //将客户端套接字加入epoll
                    // flags = fcntl(client_scoket, F_GETFL, 0);
                    // fcntl(client_scoket, F_SETFL, flags | O_NONBLOCK);
                    event.data.fd = client_scoket;
                    event.events = EPOLLIN | EPOLLET;
                    if(epoll_ctl(epfd, EPOLL_CTL_ADD, client_scoket, &event) == -1){
                        std::cerr << "客户端" << inet_ntoa(clientAddr.sin_addr) <<": epoll添加客户端套接字失败\n";
                        shutdown(client_scoket, SHUT_RDWR);
                        close(client_scoket);
                    }
                    else{
                        creatConnect(client_scoket);
                    }
                }
            }
            else{
                // std::cout << "data arrive\n";
                int client_scoket = events[i].data.fd;
                // std::cout << client_scoket << " data arrive\n";
                if(m_socketsStatus[client_scoket] == FREE){
                    Task *t = new Task;
                    t->m_args = &client_scoket;
                    t->m_run = std::bind(&Server::threadTask, this, std::placeholders::_1);
                    pool.addTask(t);
                    m_socketsStatus[client_scoket] = DEALING;
                }
                else{
                    // std::cout << "已有线程在处理当前套接字\n";
                }
            }
        }
    }
}

Data* Server::readData(int clientSocket){
    if(clientSocket == -1){
        std::cerr << "无效的客户端套接字\n";
        return nullptr;
    }
    Header header;
    int ret = read(clientSocket, &header, sizeof(Header));
    if(ret <= 0){
        closeClient(clientSocket);
        return nullptr;
    }
    Data *data = new Data;
    data->m_header = header;
    data->m_body = new char[header.m_bodySize];
    int recvLen = 0;
    while(recvLen < header.m_bodySize){
        int readLen = read(clientSocket, data->m_body + recvLen, header.m_bodySize);
        if(readLen > 0){
            recvLen += readLen;
        }
        else{
            delete data;
            data = nullptr;
            closeClient(clientSocket);
            return nullptr;            
        }
    }
    return data;
}

void Server::writeData(int clientSocket, Data &data){
    if(clientSocket == -1){
        std::cerr << "无效的客户端套接字\n";
        return;
    }
    //发送header
    signal(SIGPIPE, SIG_IGN);
    int writeLen = write(clientSocket, &data.m_header, sizeof(Header));
    if(writeLen != sizeof(Header)){
        std::cerr << "数据头部发送失败\n";
        closeClient(clientSocket);
        return;
    }
    int sendLen = 0;
    while(sendLen < data.m_header.m_bodySize){
        writeLen = write(clientSocket, data.m_body, data.m_header.m_bodySize);
        if(writeLen > 0){
            sendLen += writeLen;
        }
        else{
            std::cerr << "无法向客户端写入数据，" << strerror(errno) << std::endl;
            closeClient(clientSocket);
            return;
        }
    }
}

void Server::closeClient(int clinetSocket){
    if(clinetSocket == -1){
        return;
    }
    shutdown(clinetSocket, SHUT_RDWR);
    close(clinetSocket);
    m_mutex_clientSockets->lock();
    for(std::map<std::string, int>::iterator it = m_clientSockets.begin(); it != m_clientSockets.end(); ++it){
        if(it->second == clinetSocket){
            std::cout << it->first << "下线\n";
            m_clientSockets.erase(it);
            break;
        }
    }
    m_mutex_clientSockets->unlock();
}

bool Server::creatConnect(int clientSocket){
    if(clientSocket == -1){
        return false;
    }
    Data* data = readData(clientSocket);
    if(data == nullptr){
        std::cout << "连接建立失败\n";
        closeClient(clientSocket);
        return false;
    }
    if(data->m_header.m_dataType == USERINFO){
        m_mutex_clientSockets->lock();
        m_clientSockets[data->m_header.m_sender] = clientSocket;
        m_socketsStatus[clientSocket] = FREE;
        m_mutex_clientSockets->unlock();
        std::cout << "用户" << data->m_header.m_sender << "上线。\n";
        Data returnData;
        returnData.m_body = nullptr;
        returnData.m_header.m_bodySize = 0;
        returnData.m_header.m_dataType = USERINFO;
        strcpy(returnData.m_header.m_receiver, data->m_header.m_sender);
        strcpy(returnData.m_header.m_sender, "Server");
        writeData(clientSocket, returnData);
        delete data;
        return true;
    }
    else{
        std::cout << "连接建立失败\n";
        closeClient(clientSocket);
        delete data;
        return false;
    }
}

void Server::dealData(Data *data){
    if(data == nullptr){
        return;
    }
    int dataType = data->m_header.m_dataType;
    // std::cout << "dataType： "  << dataType << std::endl;
    if(dataType == TEXT){
        forwardData(data);
    }
    else if(dataType == IMG){
        forwardData(data);
    }
    else if(dataType == VIDEO){
        forwardData(data);
    }
    // else if(dataType == REGIST){

    // }
    // else if(dataType == LOGIN){

    // }
    else{
        std::cout << "错误的消息类型\n";
    }
}

void Server::forwardData(Data *data){
    if(data == nullptr){
        return;
    }
    std::string receiver = data->m_header.m_receiver;
    if(m_clientSockets.count(receiver) > 0){
        int destSocket = m_clientSockets[receiver];
        writeData(destSocket, *data);
    }
    else{
        std::string sender = data->m_header.m_sender;
        int destSocket = m_clientSockets[sender];
        Data d;
        const char* msg = "服务器：对方离线";
        int msgLen = strlen(msg) + 1;
        d.m_body = new char[msgLen];
        strcpy(d.m_body, msg);
        strcpy(d.m_header.m_sender, "Server");
        strcpy(d.m_header.m_receiver, sender.c_str());
        d.m_header.m_bodySize = msgLen;
        d.m_header.m_dataType = ERROR;
        writeData(destSocket, d);
    }
}

void Server::threadTask(void *socket){
    int clientSocket = *((int *)socket);
    if(clientSocket > 0){
        while(1){
            Data *data = readData(clientSocket);
            if(data != nullptr){
                dealData(data);
                delete data;
            }
            else{
                break;
            }
        }
    }
    m_socketsStatus[clientSocket] = FREE;
}

int Server::getSocket(std::string user){
    m_mutex_clientSockets->lock();
    if(m_clientSockets.count(user) > 0){
        return m_clientSockets[user];
    }
    m_mutex_clientSockets->unlock();
    return -1;
}
