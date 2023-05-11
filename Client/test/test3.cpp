#include "../src/Client.h"
#include <iostream>

int main(){
    std::string user("user3");
    Client c(user);
    c.run();
    while (1)
    {
        // Data* d = c.recvMsg();
        // if(d != nullptr){
        //     std::cout << "接收：" << d->m_body << std::endl;
        // }
        std::string msg = "我是3";
        // std::cin >> msg;
        c.sendMsg(TEXT, "user1", msg.c_str(), msg.length() + 1);
    }
}