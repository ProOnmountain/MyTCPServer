#include "../src/Client.h"
#include <iostream>

int main(){
    std::string user("user2");
    Client c(user);
    c.run();
    int i = 0;
    std::string msg = "我是" + std::to_string(i);
    while (1)
    {
        // Data* d = c.recvMsg();
        // if(d != nullptr){
        //     std::cout << "接收：" << d->m_body << std::endl;
        // }
        // std::cin >> msg;
        msg = "我是" + std::to_string(i);
        c.sendMsg(TEXT, "user1", msg.c_str(), msg.length() + 1);
        ++i;
    }
}