/**
 * @file TcpServer.cpp
 * @author Mnsx_x <xx1527030652@gmail.com>
 * @date 2026/4/9
 * @description 负责监听端口、接收连接并管理连接的生命周期
 */
#include "TcpServer.h"
#include "../include/reactor/EventLoop.h"
#include "net/Socket.h"
#include "reactor/Channel.h"
#include "Logger.h"

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

using namespace mnsx::achilles;

TcpServer::TcpServer(EventLoop *loop, uint16_t port) :
    main_loop_(loop), accept_socket_(new Socket()),
    accept_channel_(new Channel(main_loop_, accept_socket_->getFd())) {

    accept_socket_->setNonBlocking(true);
    accept_socket_->setReuseAddr(true);
    accept_socket_->setReusePort(true);

    InetAddress addr(port);
    if (!accept_socket_->bind(addr)) {
        LOG_ERROR << "TcpServer failed to bind port " << port << "!";
        exit(EXIT_FAILURE);
    }

    accept_channel_->setReadCallback([this]() {
        this->handleNewConnection();
    });

    LOG_INFO << "TcpServer initialized on port " << port;
}

TcpServer::~TcpServer() {
    LOG_INFO << "TcpServer destroyed.";
}

void TcpServer::start() {
    if (accept_socket_->listen() != true) {
        LOG_ERROR << "TcpServer failed to listen on socket!";
        exit(EXIT_FAILURE);
    }
    accept_channel_->enableReading();
    LOG_INFO << "TcpServer started listening. Epoll is waiting for connections...";
}

void TcpServer::handleNewConnection() {
    InetAddress peerAddr(0);

    int conn_fd = accept_socket_->accept(peerAddr);

    if (conn_fd >= 0) {
        LOG_INFO << "New connection accepted from "
                         << peerAddr.getIp() << ":" << peerAddr.getPort() << " [FD: " << conn_fd << "]";

        // 传概念TcpConnection对象
        auto conn = std::make_shared<TcpConnection>(main_loop_, conn_fd);

        conn->setConnectionCallback(connection_callback_);
        conn->setMessageCallback(message_callback_);
        conn->setCloseCallback([this, conn_fd](const std::shared_ptr<TcpConnection> &conn) {
            this->removeConnection(conn, conn_fd);
        });

        connections_[conn_fd] = conn;

        conn->connectEstablished();
    }
}

void TcpServer::removeConnection(const std::shared_ptr<TcpConnection> &conn, uint16_t conn_fd) {

    main_loop_->runInLoop([this, conn_fd]() {
        size_t n = this->connections_.erase(conn_fd);
        if (n != 1) {
            LOG_ERROR << "Critical Error: Attempted to remove a non-existent connection [FD: " << conn_fd << "]!";
        } else {
            LOG_DEBUG << "Connection [FD: " << conn_fd << "] removed from TcpServer.";
        }
    });
}