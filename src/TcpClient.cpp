/** 
 * @file TcpClient.cpp
 * @author Mnsx_x <xx1527030652@gmail.com>
 * @date 2026/4/9
 * @description 单个Loop客户端，支持非阻塞连接与断线重连
 */
#include "../include/TcpClient.h"
#include "../include/EventLoop.h"
#include "../src/reactor/Channel.h"

#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>

namespace mnsx {
    namespace achilles {
        TcpClient::TcpClient(EventLoop *loop, const InetAddress &server_addr) :
            loop_(loop), server_addr_(server_addr), sock_() {}

        TcpClient::~TcpClient() {
            // ...
        }

        void TcpClient::connect() {
            // 创建非阻塞的套接字
            int raw_fd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
            if (raw_fd < 0) {
                std::cerr << "TcpClient::connect() error" << std::endl;
                return;
            }
            
            // 使用智能指针维护socket的生命周期
            sock_.reset(new Socket(raw_fd));
            
            // 发起非阻塞连接
            int ret = ::connect(sock_->getFd(), server_addr_.getAddr(), sizeof(sockaddr_in));
            if (ret == 0) {
                // 本机连接会直接连接成功
                newConnection(sock_->getFd());
            } else if (errno == EINPROGRESS) {
                // 返回EINPROGRESS表示内核还在进行握手，将这个sockfd组装成Channel
                connect_channel_.reset(new Channel(loop_, sock_->getFd()));
                
                // 绑定可写事件
                connect_channel_->setWriteCallback([this]() { this->handleWrite(); });
                
                // 注册到Epoll中，管理
                connect_channel_->enableWriting();
            } else {
                // 错误
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
                // 错误
                return;
            }
            
            // 真正的连接
            newConnection(sock_->getFd());
        }

        void TcpClient::newConnection(int sock_fd) {
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
                this->connection_.reset();
            });
        }
    }
}