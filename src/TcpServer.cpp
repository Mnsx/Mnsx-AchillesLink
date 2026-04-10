/** 
 * @file TcpServer.cpp
 * @author Mnsx_x <xx1527030652@gmail.com>
 * @date 2026/4/9
 * @description 负责监听端口、接收连接并管理连接的生命周期
 */
#include "../include/TcpServer.h"
#include "../include/EventLoop.h"
#include "../src/net/Socket.h"
#include "../src/reactor/Channel.h"

#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

namespace mnsx {
    namespace achilles {
        TcpServer::TcpServer(EventLoop *loop, uint16_t port) : main_loop_(loop),
            accept_socket_(Socket::createServerSocket(port)),
            accept_channel_(std::unique_ptr<Channel>(new Channel(main_loop_, accept_socket_->getFd()))) {

            accept_channel_->setReadCallback([this]() {
                this->handleNewConnection();
            });
        }

        TcpServer::~TcpServer() {
        }

        void TcpServer::start() {
            accept_socket_->listen();
            accept_channel_->enableReading();
        }

        void TcpServer::handleNewConnection() {
            InetAddress peerAddr(0);

            int conn_fd = accept_socket_->accept(peerAddr);

            if (conn_fd >= 0) {
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
                    std::cerr << "[Mnsx-AchillesLink TcpServer] 严重警告：尝试清理不存在的连接！" << std::endl;
                }
            });
        }
    }
}