/**
 * @file TcpConnection.cpp
 * @author Mnsx_x <xx1527030652@gmail.com>
 * @date 2026/4/8
 * @description 核心连接抽象，负责单条TCP连接的状态机、缓冲区以及非阻塞I/O数据收发
 */
#include "../include/TcpConnection.h"
#include "../include/EventLoop.h"

#include <unistd.h>
#include <sys/socket.h>
#include <cerrno>
#include <iostream>

namespace mnsx {
    namespace achilles {

        TcpConnection::TcpConnection(EventLoop *loop, int conn_fd) : loop_(loop), state_(ConnectionState::CONNECTING),
            socket_(new Socket(conn_fd)), channel_(new Channel(loop, conn_fd)) {

            // 将Socket设置为非阻塞模式
            socket_->setNonBlocking(true);
            // 将处理处理事件的函数添加到Channel回调中欧给
            this->channel_->setReadCallback([this]() { this->handleRead(); });
            this->channel_->setWriteCallback([this]() { this->handleWrite(); });
            this->channel_->setCloseCallback([this]() { this->handleClose(); });
            this->channel_->setErrorCallback([this]() { this->handleError(); });
        }

        TcpConnection::~TcpConnection() {
            // Logger
        }

        void TcpConnection::connectEstablished() {
            // 修改状态机
            this->state_ = ConnectionState::CONNECTED;
            // 监听读事件
            this->channel_->enableReading();
            // 执行连接事件
            if (this->on_connection_callback_ != nullptr) {
                this->on_connection_callback_(shared_from_this());
            }
        }

        void TcpConnection::send(const std::vector<uint8_t> &data) {
            // 判断状态机，如果连接已经关闭，不允许发送
            if (this->state_ != ConnectionState::CONNECTED) {
                return;
            }
            // 判断是否在Loop线程的任务
            if (this->loop_->isInLoopThread()) {

                sendInLoop(data);
            } else {
                // 如果不是Loop线程，需要将自己的this，发给Loop线程，这就属于异步，必须使用share_from_this()
                auto self = shared_from_this();
                this->loop_->runInLoop([self, data]() {
                    self->sendInLoop(data);
                });
            }
        }

        void TcpConnection::shutdown() {
            // 判断状态机
            if (state_ == ConnectionState::CONNECTED) {
                state_ = ConnectionState::DISCONNECTED;

                // 如果发送缓冲区还没有关闭，那么需要等待写事件处理后关闭
                if (this->output_buffer_.empty()) {
                    // 所以这里使用半关闭，允许继续发送，将真正的关闭放在发送事件处理的最后
                    ::shutdown(this->socket_->getFd(), SHUT_WR);
                }
            }
        }

        void TcpConnection::sendInLoop(const std::vector<uint8_t> &data) {
            // 判断状态机
            if (state_ != ConnectionState::CONNECTED) {
                return;
            }

            ssize_t write_size = 0;
            size_t remaining = data.size();
            bool faultError = false;

            // 如果缓冲区没有内容，说明消息没有拥挤，直接通过原生代码发送
            if (output_buffer_.empty()) {
                write_size = ::write(socket_->getFd(), data.data(), data.size());
                if (write_size >= 0) {
                    remaining -= write_size;
                } else {
                    write_size = 0;
                    // 因为是非阻塞模式，所以内核缓冲区被写满后只会返回EAGAIN
                    if (errno != EAGAIN && errno != EWOULDBLOCK) {
                        // TODO Logger
                        faultError = true;
                    }
                }
            }

            // 如果是内核被装满了，那么就把剩下的装进output_buffer_中处理
            if (!faultError && remaining > 0) {
                output_buffer_.insert(output_buffer_.end(), data.begin() + write_size, data.end());
                // 开启写事件监听
                channel_->enableWriting();
            }
        }

        void TcpConnection::handleRead() {
            // 因为开启了边缘触发，所以必须读到，内核返回EAGAIN位置
            uint8_t temp_buf[65536];

            while (true) {
                ssize_t read_count = ::read(socket_->getFd(), temp_buf, sizeof(temp_buf));
                if (read_count > 0) {
                    // 将数据加入到接收缓冲区末尾
                    this->input_buffer_.insert(input_buffer_.end(), temp_buf, temp_buf + read_count);
                } else if (read_count == 0) {
                    // 返回0表示客户端断开连接
                    handleClose();
                    return;
                } else {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        // 表示已经读取完全部数据，直接黜退
                        break;
                    } else if (errno == EINTR) {
                        // 表示被系统中断信号打断，继续读取
                        continue;
                    } else {
                        // 异常
                        handleError();
                        return;
                    }
                }
            }

            // 对读取的数据进行解析
            if (!this->input_buffer_.empty()) {
                size_t parsedLength = 0;

                // 通过解析器解析数据
                while (this->parser_.parseFromRaw(this->input_buffer_.data(), this->input_buffer_.size(), parsedLength)) {

                    if (this->on_modbus_message_callback_ != nullptr) {

                        // 执行收到消息的回调，里面的TcpConnection已经存储解析过的数据
                        this->on_modbus_message_callback_(shared_from_this(), parser_);
                    }

                    // 擦除已经获取的数据报文，重置接收下一个包
                    this->input_buffer_.erase(this->input_buffer_.begin(), this->input_buffer_.begin() + parsedLength);
                    parsedLength = 0;
                }
            }
        }

        void TcpConnection::handleWrite() {
            // 如果output_buffer_不为空
            if (!output_buffer_.empty()) {
                ssize_t write_count = ::write(this->socket_->getFd(), this->output_buffer_.data(), this->output_buffer_.size());
                if (write_count > 0) {
                    // 删掉已经成功写入的数据
                    this->output_buffer_.erase(this->output_buffer_.begin(), this->output_buffer_.begin() + write_count);

                    if (this->output_buffer_.empty()) {
                        // 防止CPU空转，如果已经写完所有数据，那么关闭写事件防止空转
                        this->channel_->disableWriting();

                        if (this->state_ == ConnectionState::DISCONNECTED) {
                            // 对应shutdown中半关闭，如果状态机已关闭，并且数据写入完毕，那么推出
                            ::shutdown(this->socket_->getFd(), SHUT_WR);
                        }
                    }
                } else {
                    // TODO Logger
                    std::cerr << "...";
                }
            }
        }

        void TcpConnection::handleClose() {
            // 判断状态机已经关闭，那么直接推出
            if (this->state_ == ConnectionState::CLOSED) {
                return;
            }

            // 设置状态机
            this->state_ = ConnectionState::CLOSED;
            // 停止对所有事件的监听
            this->channel_->disableAll();

            // 执行关闭回调
            if (this->on_close_callback_!= nullptr) {
                this->on_close_callback_(shared_from_this());
            }
        }

        void TcpConnection::handleError() {
            int err = 0;
            socklen_t len = sizeof(err);
            ::getsockopt(this->socket_->getFd(), SOL_SOCKET, SO_ERROR, &err, &len);
            // TODO Logger
            std::cerr << "...";
        }

        void TcpConnection::setCloseCallback(OnCloseCallback on_close) {
            on_close_callback_ = on_close;
        }

        void TcpConnection::setMessageCallback(OnModbusMessageCallback on_message) {
            on_modbus_message_callback_ = on_message;
        }

        void TcpConnection::setConnectionCallback(OnConnectionCallback on_connection) {
            on_connection_callback_ = on_connection;
        }
    }
} // mnsx