/**
 * @file TcpConnection.h
 * @author Mnsx_x <xx1527030652@gmail.com>
 * @date 2026/4/8
 * @description 核心连接抽象，负责单条TCP连接的状态机、缓冲区以及非阻塞I/O数据收发
 */
#ifndef MNSX_ACHILLESLINK_TCPCONNECTION_H
#define MNSX_ACHILLESLINK_TCPCONNECTION_H

#include <memory>
#include <vector>
#include <functional>
#include "../src/net/Socket.h"
#include "../src/reactor/Channel.h"
#include "../src/protocol/ModbusMessage.h"

namespace mnsx {
    namespace achilles {

        enum class ConnectionState {
            CONNECTING,
            CONNECTED,
            DISCONNECTED,
            CLOSED,
            ERROR
        };

        class EventLoop;
        class TcpConnection;
        // 回调函数，数据处理回调
        using OnModbusMessageCallback = std::function<void(const std::shared_ptr<TcpConnection>&, const ModbusMessage&)>;
        // 回调函数，连接关闭回调
        using OnCloseCallback = std::function<void(const std::shared_ptr<TcpConnection>&)>;
        // 回调函数，连接建立
        using OnConnectionCallback = std::function<void(const std::shared_ptr<TcpConnection>&)>;

        // 继承
        class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
        public:
            /**
             * @brief 构造函数
             * @param loop 管理这条链接的Epoll
             * @param conn_fd 连接的句柄
             */
            TcpConnection(EventLoop* loop, int conn_fd);

            /**
             * @brief 析构函数
             */
            ~TcpConnection();

            /**
             * @delete
             */
            TcpConnection(const TcpConnection&) = delete;
            /**
             * @delete
             */
            TcpConnection& operator=(const TcpConnection&) = delete;

            /**
             * @brief 开启读事件监听，订阅连接
             */
            void connectEstablished();

            /**
             * @brief 暴露给业务层，发送数据
             * @param data
             */
            void send(const std::vector<uint8_t>& data);

            /**
             * @brief 暴露给业务层，关闭连接
             */
            void shutdown();

            /**
             * @brief Setter
             * @param cb
             */
            void setMessageCallback(const OnModbusMessageCallback& cb) { message_callback_ = cb; }
            /**
             * @brief Setter
             * @param cb
             */
            void setCloseCallback(const OnCloseCallback& cb) { close_callback_ = cb; }
            /**
             * @brief Setter
             * @param cb
             */
            void setConnectionCallback(const OnConnectionCallback& cb) { connection_callback_ = cb; }

        private:
            /**
             * @brief 发送数据底层实现
             * @param data
             */
            void sendInLoop(const std::vector<uint8_t>& data);

            /**
             * @brief 处理读事件
             */
            void handleRead();
            /**
             * @brief 处理写事件
             */
            void handleWrite();
            /**
             * @brief 处理关闭事件
             */
            void handleClose();
            /**
             * @brief 处理错误事件
             */
            void handleError();

            ConnectionState state_; // 状态

            EventLoop* loop_; // EventLoop
            std::unique_ptr<Socket> socket_; // 底层的Socket
            std::unique_ptr<Channel> channel_; // 与Epoll交互的Channel

            std::vector<uint8_t> input_buffer_; // 读取缓冲区
            std::vector<uint8_t> output_buffer_; // 写入缓冲区

            ModbusMessage parser_; // 协议解析器

            OnModbusMessageCallback message_callback_; // 消息处理回调
            OnCloseCallback close_callback_; // 调关闭回调
            OnConnectionCallback connection_callback_; // 业务层回调
        };
    }
} // mnsx

#endif //MNSX_ACHILLESLINK_TCPCONNECTION_H