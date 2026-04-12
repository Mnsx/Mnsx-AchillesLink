/** 
 * @file TcpClient.h
 * @author Mnsx_x <xx1527030652@gmail.com>
 * @date 2026/4/9
 * @description 单个Loop客户端，支持非阻塞连接与断线重连
 */
#ifndef MNSX_ACHILLESLINK_TCPCLIENT_H
#define MNSX_ACHILLESLINK_TCPCLIENT_H

#include "net/TcpConnection.h"
#include "../src/net/InetAddress.h"

#include <memory>
#include <functional>

namespace mnsx {
    namespace achilles {

        class EventLoop;
        class Channel;
        class TcpClient {
        public:
            /**
             * @brief 构造函数
             * @param loop
             * @param server_addr
             */
            TcpClient(EventLoop* loop, const InetAddress& server_addr);

            /**
             * @brief 析构函数
             */
            ~TcpClient();

            /**
             * @brief 发起非阻塞连接
             */
            void connect();

            /**
             * @brief 暴露给业务层，Setter
             * @param cb
             */
            void setConnectionCallback(const OnConnectionCallback& cb) { connection_callback_ = cb; }
            /**
             * @brief 暴露给业务层，Setter
             * @param cb
             */
            void setMessageCallback(const OnMessageCallback& cb) { message_callback_ = cb; }

        private:
            /**
             * @brief 连接成功或失败时，Epoll触发内部回调
             */
            void handleWrite();
            /**
             * @brief 握手成功后，创建真正的业务连接对象
             * @param sock_fd
             */
            void newConnection(int sock_fd);
            /**
             * @brief 清理断开的连接
             * @param conn
             */
            void removeConnection(const std::shared_ptr<TcpConnection>& conn);

            EventLoop* loop_; // 事件循环
            InetAddress server_addr_; // 服务端地址

            std::unique_ptr<Socket> sock_; // 握手阶段的临时句柄
            std::unique_ptr<Channel> connect_channel_; // 连接通道

            std::shared_ptr<TcpConnection> connection_; // 客户端只有一个维护生命周期的连接

            OnConnectionCallback connection_callback_; // 连接回调函数
            OnMessageCallback message_callback_; // 消息回调函数
        };
    }
}

#endif //MNSX_ACHILLESLINK_TCPCLIENT_H