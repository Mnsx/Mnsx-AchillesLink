/**
* @file main.cpp
 * @author Mnsx_x
 * @description Mnsx-AchillesLink 框架的业务层测试启动点
 */
#include "../include/EventLoop.h"
#include "../include/TcpServer.h"
#include "../include/TcpClient.h"
#include "../include/TcpConnection.h"
#include "../src/protocol/ModbusMessage.h" // 确保路径对应你的实际位置

#include <iostream>
#include <vector>
#include <string>

using namespace mnsx::achilles;

// 1. 定义连接建立/断开的回调函数
void onConnection(const std::shared_ptr<TcpConnection>& conn) {
    std::cout << "\n====================================" << std::endl;
    std::cout << "[业务层回调] 检测到连接状态变化！" << std::endl;
    // 如果你在 TcpConnection 中暴露了状态获取，可以在这里判断是连接还是断开
    // 也可以通过 conn->getFd() 获取句柄打印日志
    std::cout << "====================================\n" << std::endl;
}

// 2. 定义收到消息的回调函数
void onMessage(const std::shared_ptr<TcpConnection>& conn, const ModbusMessage& parser) {
    std::cout << "[业务层回调] 收到完整的 Modbus 报文，正在处理..." << std::endl;

    // 模拟处理耗时业务，并回传数据
    // ⚠️ 注意：在我们后续接入 KrakenPool 后，这段逻辑会被扔进线程池执行
    // 但因为你的 conn->send() 已经做了跨线程安全保护，所以以后怎么调都不怕！

    // 伪造一个 Modbus 响应报文 (例如: 设备地址 0x01, 功能码 0x03)
    std::vector<uint8_t> mock_response = {0x01, 0x03, 0x02, 0x00, 0x05, 0x00};

    // 调用我们在 TcpConnection 中写好的跨线程安全发送函数
    conn->send(mock_response);

    std::cout << "[业务层回调] 响应数据已塞入发送队列！" << std::endl;

    std::string reply = "Server Received!\n";
    std::vector<uint8_t> reply_data(reply.begin(), reply.end());
    conn->send(reply_data);
}

// ... 前面保持你原来的 TcpServer 测试代码 ...

int main() {
    EventLoop loop;

    // 1. 启动你的 Server
    TcpServer server(&loop, 8080);
    server.setConnectionCallback(onConnection);
    server.setMessageCallback(onMessage);
    server.start();

    // 2. 启动你的 Client (复用同一个心脏！)
    // 创建一个指向本机的地址
    InetAddress server_addr(8080, "127.0.0.1");
    TcpClient client(&loop, server_addr);

    // 给 Client 也设置回调
    client.setConnectionCallback([](const std::shared_ptr<TcpConnection>& conn) {
        // 如果连接成功，Client 主动发一条消息打招呼
        std::vector<uint8_t> hello = {'H', 'e', 'l', 'l', 'o', '\n'};
        conn->send(hello);
    });

    client.setMessageCallback([](const std::shared_ptr<TcpConnection>& conn, const ModbusMessage& parser) {
        std::cout << "[Client端] 收到了来自 Server 的回复！" << std::endl;
    });

    // 3. Client 发起连接
    client.connect();

    // 4. 引擎启动
    loop.loop();

    return 0;
}