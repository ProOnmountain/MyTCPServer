#pragma once
#include <string>
#include <vector>
#include <map>
#include <mutex>

class Data;
class Server{
public:
    Server();
    ~Server();
    void run();
    Data* readData(int clientSocket);
    void writeData(int clientSocket, Data &data);
    int getSocket(std::string user);
    void threadTask(void *socket);

private:
    bool config(std::string configFile);
    void initSocket();
    void runModeThreads();//普通模式
    void runModeThreadPool();//epoll模式
    void closeClient(int clientSocket);
    bool creatConnect(int clientSocket);//和客户端建立连接（将用户信息和套接字对应起来，并返回信息给用户）
    void dealData(Data* data);
    void forwardData(Data* data);
    

private:
    std::map<std::string, int> m_clientSockets; //记录和服务器建立连接的客户端
    int m_serverSocket;
    std::string m_ip;
    int m_port;
    int m_mode;
    std::mutex *m_mutex_clientSockets;
    int m_threadNum;
    std::map<int, int> m_socketsStatus;
};