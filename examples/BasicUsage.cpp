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

using namespace mnsx::achilles;

void onConnection(const std::shared_ptr<TcpConnection>& conn) {
    std::cout << "stat" << std::endl;
}

void onMessage(const std::shared_ptr<TcpConnection>& conn, const ModbusMessage& parser) {

    uint16_t tid = parser.getTransactionId();
    uint8_t  uid = parser.getUnitId();
    uint8_t  fc  = parser.getFunctionCode();

    const std::vector<uint8_t>& payload = parser.getPayload();

    std::cout << "------------------------------------" << std::endl;
    std::printf("Transaction ID: 0x%X\n", tid);
    std::printf("Unit ID:        0x%X\n", uid);
    std::printf("Function Code:  0x%X\n", fc);
    std::cout << "Payload Size:   " << payload.size() << " bytes" << std::endl;
    std::cout << "Payload Data:   ";
    for (auto byte : payload) {
        std::printf("%02X ", byte);
    }
    std::cout << "\n------------------------------------" << std::endl;
}

void onRecMessage(const std::shared_ptr<TcpConnection>& conn, const ModbusMessage& parser) {
    uint16_t tid = parser.getTransactionId();
    uint8_t  uid = parser.getUnitId();

    uint16_t fakeValue = 1234;

    std::vector<uint8_t> res;

    uint16_t n_tid = htons(tid);
    uint16_t n_len = htons(5);
    uint16_t n_pid = 0;

    res.insert(res.end(), (uint8_t*)&n_tid, (uint8_t*)&n_tid + 2);
    res.insert(res.end(), (uint8_t*)&n_pid, (uint8_t*)&n_pid + 2);
    res.insert(res.end(), (uint8_t*)&n_len, (uint8_t*)&n_len + 2);
    res.push_back(uid);

    res.push_back(0x03);
    res.push_back(0x02);
    res.push_back(fakeValue >> 8);
    res.push_back(fakeValue & 0xFF);

    conn->send(res);
}

int main() {
    EventLoop loop;

    TcpServer server(&loop, 8080);
    server.setConnectionCallback(onConnection);
    server.setMessageCallback([](const std::shared_ptr<TcpConnection>& conn, const ModbusMessage& parser) {

        onRecMessage(conn, parser);
    });
    server.start();

    InetAddress server_addr(8088, "172.25.80.1");
    TcpClient client(&loop, server_addr);

    client.setConnectionCallback([](const std::shared_ptr<TcpConnection>& conn) {
        std::vector<uint8_t> data = {
                0x03, // 功能码
                // 0x06,
                0x00, 0x00, // 起始地址
                0x00, 0x01 // 获取长度 / 写入的数据
            };

            MbapHeader header{};
            header.transaction_id = htons(0x0001);
            header.protocol_id    = htons(0x0000);
            header.length         = htons(1 + data.size());
            header.unit_id        = 0x01;

            std::vector<uint8_t> send_data;
            auto header_ptr = reinterpret_cast<uint8_t*>(&header);
            send_data.insert(send_data.end(), header_ptr, header_ptr + sizeof(MbapHeader));
            send_data.insert(send_data.end(), data.begin(), data.end());

            conn->send(send_data);
    });

    client.setMessageCallback([](const std::shared_ptr<TcpConnection>& conn, const ModbusMessage& parser) {
        onMessage(conn, parser);
    });

    client.connect();

    loop.loop();

    return 0;
}