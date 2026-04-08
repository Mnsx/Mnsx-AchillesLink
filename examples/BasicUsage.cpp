/**
* @file main.cpp
 * @author Mnsx_x
 * @description Mnsx-AchillesLink TcpServer 测试入口
 */
#include "../src/test/TcpServer.h"
#include "../src/reactor/Epoll.h"      // 假设你的 Epoll 类在这里
#include "../src/net/TcpConnection.h"
#include <iostream>
#include <string>
#include <vector>

using namespace mnsx::achilles;

int main() {
    // 1. 创建主事件循环 (Main Reactor)
    Epoll epoll;

    // 2. 创建 TcpServer 实例，监听 8080 端口
    uint16_t port = 8080;
    TcpServer server(&epoll, port);

    // 3. 注册【连接状态改变】的回调函数
    server.setConnectionCallback([](const std::shared_ptr<TcpConnection>& conn) {
        // 由于你的 state_ 可能没有暴露 getter，假设你有个类似 isConnected() 的方法
        // 这里简化演示：只要回调触发且 fd 有效，我们就打个招呼

        // 主动向刚连接的客户端发送欢迎信息 (Server Push)
        std::string welcome = "Welcome to Mnsx-AchillesLink Server!\n";
        std::vector<uint8_t> data(welcome.begin(), welcome.end());

        // 注意：这里需要确保你的 conn 处于 CONNECTED 状态才能发出去
        conn->send(data);
    });

    // 4. 注册【收到完整报文】的回调函数
    server.setMessageCallback([](const std::shared_ptr<TcpConnection>& conn, const ModbusMessage& parser) {
        std::cout << "[业务层] 收到完整解析的报文，准备回复..." << std::endl;

        // 【测试逻辑】：无论收到什么 Modbus 报文，我们都回复一个 "OK"
        std::string reply = "Server Received: OK\n";
        std::vector<uint8_t> respData(reply.begin(), reply.end());

        conn->send(respData);
    });

    // 5. 启动服务器（开始监听端口）
    server.start();

    std::cout << "==========================================\n";
    std::cout << "Mnsx Server is running on port " << port << "\n";
    std::cout << "Waiting for connections...\n";
    std::cout << "==========================================\n";

    return 0;
}