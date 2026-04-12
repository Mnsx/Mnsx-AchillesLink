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

#include "../../src/net/Socket.h"
#include "../../src/net/ByteBuffer.h"
#include "../../src/reactor/Channel.h"

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
        using OnMessageCallback = std::function<void(const std::shared_ptr<TcpConnection>&, ByteBuffer*)>;
        // 回调函数，连接关闭回调
        using OnCloseCallback = std::function<void(const std::shared_ptr<TcpConnection>&)>;
        // 回调函数，连接建立
        using OnConnectionCallback = std::function<void(const std::shared_ptr<TcpConnection>&)>;

        class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
        public:
            /**
             * 构造函数，为连接绑定Loop
             * @param loop
             * @param conn_fd
             */
            TcpConnection(EventLoop* loop, int conn_fd);
            /**
             * 析构函数
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
             * 核心，为当前连接发布订阅，开启对应这个连接的Channel的读事件的监听
             */
            void connectEstablished();

            /**
             * 暴露给逻辑层，用来发送数据
             * @param message
             */
            void send(const std::string& message);
            /**
             * 暴露给逻辑层，用来发送数据
             * @param message
             * @param len
             */
            void send(const char* message, size_t len);

            /**
             * 暴露给逻辑层，用来关闭
             */
            void shutdown();

            /**
             * @brief input_buffer_ Getter
             * @return
             */
            ByteBuffer* getInputBuffer() { return &input_buffer_; }
            /**
             * @brief output_buffer_ Getter
             * @return
             */
            ByteBuffer* getOutputBuffer() {return &output_buffer_; }

            /**
             * 消息处理回调
             * @param on_message
             */
            void setMessageCallback(OnMessageCallback on_message);
            /**
             * 关闭回调
             * @param on_close
             */
            void setCloseCallback(OnCloseCallback on_close);
            /**
             * 连接回调
             * @param on_connection
             */
            void setConnectionCallback(OnConnectionCallback on_connection);

        private:
            /**
             * 发送数据底层实现
             * @param data
             * @param len
             */
            void sendInLoop(const char* data, size_t len);

            /**
             * 处理对事件
             */
            void handleRead();
            /**
             * 处理写事件
             */
            void handleWrite();
            /**
             * 处理关闭事件
             */
            void handleClose();
            /**
             * 处理错误事件
             */
            void handleError();

            ConnectionState state_; // 状态机

            EventLoop* loop_; // 绑定的EventLoop
            std::unique_ptr<Socket> socket_; // 当前连接对应的Socket，不是线程使用，使用智能指针保证生命周期
            std::unique_ptr<Channel> channel_; // 当前连接对应的Channel

            ByteBuffer input_buffer_; // 读取缓冲区
            ByteBuffer output_buffer_; // 写入缓冲区

            // 回调函数，暴露给业务层
            OnMessageCallback on_modbus_message_callback_;
            OnCloseCallback on_close_callback_;
            OnConnectionCallback on_connection_callback_;
        };

    }
} // mnsx

#endif //MNSX_ACHILLESLINK_TCPCONNECTION_H