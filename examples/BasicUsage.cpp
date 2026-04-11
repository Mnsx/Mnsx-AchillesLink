/**
 * @file BasicUsage.cpp
 * @author Mnsx_x
 * @description Mnsx-AchillesLink 框架的业务层测试
 */
#include "../include/EventLoop.h"
#include "../include/TcpServer.h"
#include "../include/TcpClient.h"
#include "../include/TcpConnection.h"
#include "../src/protocol/ModbusMessage.h"

#include <iostream>
#include <vector>
#include <string>

#include "Logger.h"
#include "../src/net/ByteBuffer.h"

using namespace mnsx::achilles;

void onConnection(const std::shared_ptr<TcpConnection>& conn) {
}

void onMessage(const std::shared_ptr<TcpConnection>& conn, ByteBuffer* buffer) {

    std::string res =  buffer->retrieveAllAsString();
    LOG_DEBUG << res;
}

void onRecMessage(const std::shared_ptr<TcpConnection>& conn, ByteBuffer* buffer) {

    std::string res =  buffer->retrieveAllAsString();
    LOG_DEBUG << res;
    conn->send(std::move("Byte!"));
}

int main() {

    EventLoop loop;

    TcpServer server(&loop, 8080);
    server.setConnectionCallback(onConnection);
    server.setMessageCallback([](const std::shared_ptr<TcpConnection>& conn, ByteBuffer* buffer) {
        onRecMessage(conn, buffer);
    });
    server.start();

    InetAddress server_addr(8080, "127.0.0.1");
    TcpClient client(&loop, server_addr);

    client.setConnectionCallback([](const std::shared_ptr<TcpConnection>& conn) {
            conn->send(std::move("Hello!"));
    });

    client.setMessageCallback([](const std::shared_ptr<TcpConnection>& conn, ByteBuffer* buffer) {
        onMessage(conn, buffer);
    });

    client.connect();

    loop.loop();

    return 0;
}