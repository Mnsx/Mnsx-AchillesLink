/**
 * @file BasicUsage.cpp
 * @author Mnsx_x
 * @description Mnsx-AchillesLink 框架的业务层测试
 */
#include "../include/reactor/EventLoop.h"
#include "../include/TcpServer.h"
#include "../include/TcpClient.h"
#include "../include/net/TcpConnection.h"
#include <iostream>
#include <vector>
#include <string>

#include "Logger.h"
#include "../src/net/ByteBuffer.h"
#include "../include/protocol/modbus/ModbusCodec.h"
#include "protocol/http/HttpCodec.h"
#include "protocol/rpc/RpcCodec.h"

using namespace mnsx::achilles;

// ====================================================================
// 服务端业务逻辑
// ====================================================================
void onServerMessage(const std::shared_ptr<TcpConnection>& conn, ByteBuffer* buf) {
    // std::string frame;

    // // 1. 使用 Codec 的纯粹 decode，循环解包，完美防御粘包！
    // while (ModbusCodec::decode(buffer, frame)) {
    //
    //     // 提取 PDU (跳过前 7 个字节的 MBAP 头，获取真实的业务数据)
    //     std::string pdu = frame.size() > 7 ? frame.substr(7) : "";
    //     LOG_INFO << "[Server] Received Modbus Frame! Total Size: " << frame.size()
    //              << ", PDU Payload: " << pdu;
    //
    //     // 2. 模拟业务处理，准备发送回执
    //     std::string response_pdu = "Server Ack: " + pdu;
    //     ByteBuffer outBuf;
    //
    //     // 3. 使用 Codec 的纯粹 encode 打包响应
    //     // 参数: TransID=1001, UnitID=1, PDU数据, 输出缓冲区
    //     ModbusCodec::encode(1001, 1, response_pdu, &outBuf);
    //
    //     // 4. 交给网络层发送 (取出打好包的数据)
    //     conn->send(outBuf.retrieveAllAsString());
    // }


    // HttpRequest req;
    //
    // // 利用 while 循环处理 Http 管道化 (Pipeline) 和粘包
    // while (HttpCodec::decode(buf, req)) {
    //
    //     LOG_INFO << "Received HTTP Request: " << req.method << " " << req.path;
    //
    //     // --- 业务路由逻辑 ---
    //     HttpResponse resp;
    //
    //     if (req.path == "/hello") {
    //         resp.statusCode = 200;
    //         resp.body = "<h1>Hello from AchillesLink!</h1><h2>我是你爹 </h2><p>Your method is: " + req.method + "</p>";
    //         resp.headers["Content-Type"] = "text/html; charset=utf-8";
    //     } else if (req.path == "/api/data") {
    //         resp.statusCode = 200;
    //         resp.body = "{\"status\": \"success\", \"message\": \"JSON Data\"}";
    //         resp.headers["Content-Type"] = "application/json";
    //     } else {
    //         resp.statusCode = 404;
    //         resp.statusMessage = "Not Found";
    //         resp.body = "404 Page Not Found";
    //     }
    //
    //     // --- 打包响应并发送 ---
    //     ByteBuffer outBuf;
    //     HttpCodec::encode(resp, &outBuf);
    //     conn->send(outBuf.retrieveAllAsString());

        // 注意：如果是 HTTP/1.0 或者请求头里有 Connection: close，
        // 这里发送完数据后，应该主动调用 conn->shutdown() 断开连接。
    // }

    // ======================================================
    // RPC
    // ======================================================
    RpcMessage req;
    RpcCodec::decode(buf, req);
    // 纯手工字符串切割
    std::string body = req.body; // "Login|mnsx|123456"
    size_t first_pipe = body.find('|');
    size_t second_pipe = body.find('|', first_pipe + 1);

    std::string method_name = body.substr(0, first_pipe);
    std::string username = body.substr(first_pipe + 1, second_pipe - first_pipe - 1);
    std::string password = body.substr(second_pipe + 1);

    if (method_name == "Login") {
        LOG_INFO << "Executing Login... User: " << username;
        // ... 调用数据库 ...
    }
}

// ====================================================================
// 客户端业务逻辑
// ====================================================================
void onClientMessage(const std::shared_ptr<TcpConnection>& conn, ByteBuffer* buffer) {
    // std::string frame;
    //
    // while (ModbusCodec::decode(buffer, frame)) {
    //     std::string pdu = frame.size() > 7 ? frame.substr(7) : "";
    //     LOG_INFO << "[Client] Received Modbus Response! Total Size: " << frame.size()
    //              << ", PDU Payload: " << pdu;
    // }
}

int main() {
    // 1. 初始化事件循环
    EventLoop loop;

    // ================== 启动 Server ==================
    TcpServer server(&loop, 8080);
    server.setConnectionCallback([](const std::shared_ptr<TcpConnection>& conn) {
        LOG_INFO << "[Server] New connection established!";
    });
    // 绑定服务端的解析流水线
    server.setMessageCallback(onServerMessage);
    server.start();

    // ================== 启动 Client ==================
    InetAddress server_addr(8080, "127.0.0.1");
    TcpClient client(&loop, server_addr);

    client.setConnectionCallback([](const std::shared_ptr<TcpConnection>& conn) {
        RpcMessage req;
        req.request_id = 1;
        // 采用最暴力的竖线分割法： 方法名 | 参数1 | 参数2
        req.body = "Login|mnsx|123456";

        ByteBuffer outBuf;
        RpcCodec::encode(req, &outBuf);
        conn->send(outBuf.retrieveAllAsString());
    });

    // // 客户端连接成功后的动作
    // client.setConnectionCallback([](const std::shared_ptr<TcpConnection>& conn) {
    // LOG_INFO << "[Client] Connected to server! Sending first Modbus request...";
    //
    // // 使用 Codec 打包一条 Modbus 请求发过去
    // ByteBuffer outBuf;
    // ModbusCodec::encode(1, 255, "Hello Server! This is Modbus Protocol.", &outBuf);
    // conn->send(outBuf.retrieveAllAsString());
    // });

    // 绑定客户端的解析流水线
    client.setMessageCallback(onClientMessage);
    client.connect();

    // 2. 开启引擎
    loop.loop();

    return 0;
}