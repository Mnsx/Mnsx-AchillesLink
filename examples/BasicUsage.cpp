/**
* @file main.cpp
 * @author Mnsx_x
 * @description Mnsx-AchillesLink 框架的业务层测试启动点
 */
#include "../include/EventLoop.h"
#include "../include/TcpServer.h"
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

int main() {

    setvbuf(stdout, nullptr, _IONBF, 0);
    std::cout << "========== Mnsx-AchillesLink ==========" << std::endl;
    std::cout << "初始化 EventLoop..." << std::endl;

    // 1. 创建整个应用唯一的心脏
    EventLoop loop;

    // 2. 创建服务器总指挥，绑定 Loop，监听 8080 端口
    TcpServer server(&loop, 8080);

    // 3. 注册我们刚才写的业务回调
    server.setConnectionCallback(onConnection);
    server.setMessageCallback(onMessage);

    // 4. 启动服务端 (此时只是开始 listen 并注册事件，不会阻塞)
    server.start();

    std::cout << "引擎点火！服务器正在 8080 端口死循环监听..." << std::endl;
    std::cout << "=======================================" << std::endl;

    // 5. 正式死循环，接管所有网络 I/O 和跨线程任务
    loop.loop();

    return 0;
}