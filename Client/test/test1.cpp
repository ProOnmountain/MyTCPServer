#include "../src/Client.h"
#include <iostream>

int main(){
    std::string user("user1");
    Client c(user);
    c.run();
    while (1)
    {
        // std::string msg = "我是1";
        // // std::cin >> msg;
        // c.sendMsg(TEXT, "user2", msg.c_str(), msg.length() + 1);
        Data* d = c.recvMsg();
        if(d != nullptr){
            std::cout << "接收：" << d->m_body << std::endl;
        }
        delete d;
    }
}