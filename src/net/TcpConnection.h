/**
 * @file TcpConnection.h
 * @author Mnsx_x <xx1527030652@gmail.com>
 * @date 2026/4/8
 * @description
 */
#ifndef MNSX_ACHILLESLINK_TCPCONNECTION_H
#define MNSX_ACHILLESLINK_TCPCONNECTION_H

#include <memory>
#include <vector>
#include <functional>
#include "Socket.h"
#include "../reactor/Channel.h"
#include "../protocol/ModbusMessage.h"

namespace mnsx {
    namespace achilles {

        enum class ConnectionState {
            CONNECTING,
            CONNECTED,
            DISCONNECTED,
            CLOSED,
            ERROR
        };

        class Epoll;
        class TcpConnection;

        using OnModbusMessageCallback = std::function<void(const std::shared_ptr<TcpConnection>&, const ModbusMessage&)>;
        using OnCloseCallback = std::function<void(const std::shared_ptr<TcpConnection>&)>;

        class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
        public:
            TcpConnection(Epoll* epoll, int conn_fd);
            ~TcpConnection();

            TcpConnection(const TcpConnection&) = delete;
            TcpConnection& operator=(const TcpConnection&) = delete;

            void connectEstablished();

            void send(const std::vector<uint8_t>& data);

            void shutdown();

            void setMessageCallback(const OnModbusMessageCallback& cb) { message_callback_ = cb; }
            void setCloseCallback(const OnCloseCallback& cb) { close_callback_ = cb; }

        private:
            void handleRead();
            void handleWrite();
            void handleClose();
            void handleError();

            ConnectionState state_; // 状态

            Epoll* epoll_;
            std::unique_ptr<Socket> socket_;
            std::unique_ptr<Channel> channel_;

            std::vector<uint8_t> input_buffer_;
            std::vector<uint8_t> output_buffer_;

            ModbusMessage parser_;

            OnModbusMessageCallback message_callback_;
            OnCloseCallback close_callback_;
        };
    }
} // mnsx

#endif //MNSX_ACHILLESLINK_TCPCONNECTION_H