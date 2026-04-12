/** 
 * @file TcpServer.h
 * @author Mnsx_x <xx1527030652@gmail.com>
 * @date 2026/4/9
 * @description 负责监听端口、接收连接并管理连接的生命周期
 */
#ifndef MNSX_ACHILLESLINK_TCPSERVER_H
#define MNSX_ACHILLESLINK_TCPSERVER_H

#include <map>
#include <memory>
#include <functional>

#include "net/TcpConnection.h"
#include "../src/net/ByteBuffer.h"

namespace mnsx {
    namespace achilles {

        class EventLoop;
        class Socket;
        class Channel;
        class TcpServer {
        public:
            using ConnectionCallback = std::function<void(const std::shared_ptr<TcpConnection>&)>;
            using MessageCallback = std::function<void(const std::shared_ptr<TcpConnection>&, ByteBuffer*)>;

            /**
             * @brief 构造函数
             * @param loop
             * @param port
             */
            TcpServer(EventLoop *loop, uint16_t port);
            /**
             * @brief 析构函数
             */
            ~TcpServer();

            /**
             * @brief 启动服务端
             */
            void start();

            /**
             * @brief Setter
             * @param cb
             */
            void setConnectionCallback(ConnectionCallback cb) {
                this->connection_callback_ = std::move(cb);
            }
            /**
             * @brief Setter
             * @param cb
             */
            void setMessageCallback(MessageCallback cb) {
                this->message_callback_ = std::move(cb);
            }

        private:
            /**
             * @brief 处理新客户端连接
             */
            void handleNewConnection();

            /**
             * @brief 清除断开连接
             * @param conn
             */
            void removeConnection(const std::shared_ptr<TcpConnection>& conn, uint16_t conn_fd);

            EventLoop* main_loop_; // server的EventLoop（唯一）

            std::unique_ptr<Socket> accept_socket_; // 服务端Socket
            std::unique_ptr<Channel> accept_channel_; // 服务端Channel

            ConnectionCallback connection_callback_; // 业务层，连接回调
            MessageCallback message_callback_; // 业务层，消息回调
            
            std::map<int, std::shared_ptr<TcpConnection>> connections_;
        };
    }
}

#endif //MNSX_ACHILLESLINK_TCPSERVER_H