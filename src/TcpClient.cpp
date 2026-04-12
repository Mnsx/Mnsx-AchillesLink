/** 
 * @file TcpClient.cpp
 * @author Mnsx_x <xx1527030652@gmail.com>
 * @date 2026/4/9
 * @description 单个Loop客户端，支持非阻塞连接与断线重连
 */
#include "TcpClient.h"
#include "../include/reactor/EventLoop.h"
#include "reactor/Channel.h"
#include "Logger.h"

#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>

using namespace mnsx::achilles;

TcpClient::TcpClient(EventLoop *loop, const InetAddress &server_addr) :
    loop_(loop), server_addr_(server_addr), sock_() {}

TcpClient::~TcpClient() {
    LOG_DEBUG << "TcpClient connecting to " << server_addr_.getIp() << ":" << server_addr_.getPort() << " destroyed.";
}

void TcpClient::connect() {
    sock_.reset(new Socket());
    sock_->setNonBlocking(true); // 必须非阻塞

    LOG_INFO << "TcpClient trying to connect to " << server_addr_.getIp() << ":" << server_addr_.getPort() << "...";

    // 发起非阻塞连接
    int ret = ::connect(sock_->getFd(), server_addr_.getAddr(), sizeof(sockaddr_in));
    if (ret == 0) {
        // 本机连接会直接连接成功
        LOG_INFO << "TcpClient connected immediately!";
        newConnection(sock_->getFd());
    } else if (errno == EINPROGRESS) {
        LOG_DEBUG << "TcpClient connect is in progress (EINPROGRESS). Waiting for Epoll writable event...";
        // 返回EINPROGRESS表示内核还在进行握手，将这个sockfd组装成Channel
        connect_channel_.reset(new Channel(loop_, sock_->getFd()));

        // 绑定可写事件
        connect_channel_->setWriteCallback([this]() { this->handleWrite(); });

        // 注册到Epoll中，管理
        connect_channel_->enableWriting();
    } else {
        LOG_ERROR << "TcpClient connect failed instantly! Reason: " << strerror(errno);
        // TODO 重连
    }
}

void TcpClient::handleWrite() {
    // 注销监听，因为这次监听是，接收握手的结果
    connect_channel_->disableAll();
    loop_->removeChannel(connect_channel_.get());

    // 验证连接
    int err = 0;
    socklen_t len = sizeof(err);
    if (::getsockopt(sock_->getFd(), SOL_SOCKET, SO_ERROR, &err, &len) < 0 || err != 0) {
        err = errno;
    }

    if (err != 0) {
        LOG_ERROR << "TcpClient async connect failed! Reason: " << strerror(err);
        // TODO 重连
        return;
    }

    // 真正的连接
    LOG_INFO << "TcpClient async connection established successfully!";
    newConnection(sock_->getFd());
}

void TcpClient::newConnection(int sock_fd) {
    connect_channel_.reset();

    connection_ = std::make_shared<TcpConnection>(loop_, sock_fd);

    connection_->setConnectionCallback(connection_callback_);
    connection_->setMessageCallback(message_callback_);
    connection_->setCloseCallback([this](const std::shared_ptr<TcpConnection>& conn) {

        this->removeConnection(conn);
    });

    // 激活链接状态机
    connection_->connectEstablished();
}

void TcpClient::removeConnection(const std::shared_ptr<TcpConnection> &conn) {
    loop_->runInLoop([this]() {
        LOG_INFO << "TcpClient connection closed.";
        this->connection_.reset();
    });
}