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

    // while (ModbusCodec::decode(buffer, frame)) {
    //
    //     std::string pdu = frame.size() > 7 ? frame.substr(7) : "";
    //     LOG_INFO << "[Server] Received Modbus Frame! Total Size: " << frame.size()
    //              << ", PDU Payload: " << pdu;
    //
    //     std::string response_pdu = "Server Ack: " + pdu;
    //     ByteBuffer outBuf;
    //
    //     // 参数: TransID=1001, UnitID=1, PDU数据, 输出缓冲区
    //     ModbusCodec::encode(1001, 1, response_pdu, &outBuf);
    //
    //     conn->send(outBuf.retrieveAllAsString());
    // }


    HttpRequest req;

    while (HttpCodec::decode(buf, req)) {

        LOG_INFO << "Received HTTP Request: " << req.method << " " << req.path;

        HttpResponse resp;

        if (req.path == "/hello") {
            resp.statusCode = 200;
            resp.body = "<h1>Hello from AchillesLink!</h1><p>Your method is: " + req.method + "</p>";
            resp.headers["Content-Type"] = "text/html; charset=utf-8";
        } else if (req.path == "/api/data") {
            resp.statusCode = 200;
            resp.body = "{\"status\": \"success\", \"message\": \"JSON Data\"}";
            resp.headers["Content-Type"] = "application/json";
        } else {
            resp.statusCode = 404;
            resp.statusMessage = "Not Found";
            resp.body = "404 Page Not Found";
        }

        ByteBuffer outBuf;
        HttpCodec::encode(resp, &outBuf);
        conn->send(outBuf.retrieveAllAsString());
    }

    // ======================================================
    // RPC
    // ======================================================
    // RpcMessage req;
    // RpcCodec::decode(buf, req);
    // std::string body = req.body; // "Login|mnsx|123456"
    // size_t first_pipe = body.find('|');
    // size_t second_pipe = body.find('|', first_pipe + 1);
    //
    // std::string method_name = body.substr(0, first_pipe);
    // std::string username = body.substr(first_pipe + 1, second_pipe - first_pipe - 1);
    // std::string password = body.substr(second_pipe + 1);
    //
    // if (method_name == "Login") {
    //     LOG_INFO << "Executing Login... User: " << username;
    // }
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
    EventLoop loop;

    // ================== 启动 Server ==================
    TcpServer server(&loop, 8080);
    server.setConnectionCallback([](const std::shared_ptr<TcpConnection>& conn) {
        LOG_INFO << "[Server] New connection established!";
    });
    server.setMessageCallback(onServerMessage);
    server.start();

    // ================== 启动 Client ==================
    // InetAddress server_addr(8080, "127.0.0.1");
    // TcpClient client(&loop, server_addr);
    //
    // client.setConnectionCallback([](const std::shared_ptr<TcpConnection>& conn) {
    //     RpcMessage req;
    //     req.request_id = 1;
    //     req.body = "Login|mnsx|123456";
    //
    //     ByteBuffer outBuf;
    //     RpcCodec::encode(req, &outBuf);
    //     conn->send(outBuf.retrieveAllAsString());
    // });

    // ===========================================================================================
    // modbus
    // ===========================================================================================
    // client.setConnectionCallback([](const std::shared_ptr<TcpConnection>& conn) {
    // LOG_INFO << "[Client] Connected to server! Sending first Modbus request...";

    // ByteBuffer outBuf;
    // ModbusCodec::encode(1, 255, "Hello Server! This is Modbus Protocol.", &outBuf);
    // conn->send(outBuf.retrieveAllAsString());
    // });


    // client.setMessageCallback(onClientMessage);
    // client.connect();

    loop.loop();

    return 0;
}