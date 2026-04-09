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
        TcpConnection::TcpConnection(EventLoop* loop, int conn_fd) : loop_(loop), state_(ConnectionState::CONNECTING),
            socket_(new Socket(conn_fd)), channel_(new Channel(loop, conn_fd)) {

            // 将Socket设置为非阻塞模式
            this->socket_->setNonBlocking(true);

            // 将处理事件的函数添加到Channel回调中
            this->channel_->setReadCallback([this]() { this->handleRead(); });
            this->channel_->setWriteCallback([this]() { this->handleWrite(); });
            this->channel_->setCloseCallback([this]() { this->handleClose(); });
            this->channel_->setErrorCallback([this]() { this->handleError(); });
        }

        TcpConnection::~TcpConnection() {
            // TODO 日志部分处理
            std::cout << "[Mnsx-AchillesLink TcpConnection] 连接已销毁，fd：" << this->socket_->getFd() << std::endl;
        }

        void TcpConnection::connectEstablished() {
            this->state_ = ConnectionState::CONNECTED;
            // 监听读取事件
            this->channel_->enableReading();
        }

        void TcpConnection::send(const std::vector<uint8_t> &data) {

            if (this->state_ != ConnectionState::CONNECTED) {
                return;
            }

            if (this->loop_->isInLoopThread()) {

                // 直接发送
                sendInLoop(data);
            } else {

                auto self = shared_from_this();
                this->loop_->runInLoop([self, data]() {
                    self->sendInLoop(data);
                });
            }
        }

        void TcpConnection::shutdown() {

            if (this->state_ == ConnectionState::CONNECTED) {
                this->state_ = ConnectionState::DISCONNECTED;

                // 如果发送缓冲区还未关闭，那么会等待handle_write执行完毕后，关闭
                if (this->output_buffer_.empty()) {
                    ::shutdown(this->socket_->getFd(), SHUT_WR);
                }
            }
        }

        void TcpConnection::sendInLoop(const std::vector<uint8_t> &data) {

            if (this->state_ != ConnectionState::CONNECTED) {
                // 连接已经断开拒绝读取
                return;
            }

            ssize_t write_size = 0;
            size_t remaining = data.size();
            bool faultError = false;

            // 如果缓冲区没有内容，那么说明没有排队，直接通过原生代码发送
            if (this->output_buffer_.empty()) {
                write_size = ::write(this->socket_->getFd(), data.data(), data.size());
                if (write_size >= 0) {
                    remaining -= write_size;
                } else {
                    write_size = 0;
                    // 因为是非阻塞模式，所以内核缓冲区被写满只会返回EAGAIN不会报错
                    if (errno != EAGAIN && errno != EWOULDBLOCK) {
                        std::cerr << "...";
                        faultError = true;
                    }
                }
            }

            // 如果内核缓冲区被装满了，把剩下的装入到应用层缓冲区中
            if (!faultError && remaining > 0) {
                this->output_buffer_.insert(this->output_buffer_.end(), data.data() + write_size, data.data() + data.size());
                channel_->enableWriting();
            }
        }

        void TcpConnection::handleRead() {
            // 边缘触发，必须读到返回EAGAIN为止
            uint8_t tempBuf[65536];

            while (true) {
                ssize_t n = ::read(this->socket_->getFd(), tempBuf, sizeof(tempBuf));
                if (n > 0) {
                    // 将数据加入到接收缓冲区末尾
                    this->input_buffer_.insert(this->input_buffer_.end(), tempBuf, tempBuf + n);
                } else if (n == 0) {
                    // 返回0代表客户端主动断开连接
                    handleClose();
                    return;
                } else {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        // 已经读取完毕
                        break;
                    } else if (errno == EINTR) {
                        // 被系统中断信号打断，继续读
                        continue;
                    } else {
                        // 异常
                        handleError();
                        return;
                    }
                }
            }

            // 处理缓冲区数据
            if (!this->input_buffer_.empty()) {
                size_t parsedLength = 0;

                // while (this->parser_.parseFromRaw(this->input_buffer_.data(), this->input_buffer_.size(), parsedLength)) {

                    if (this->message_callback_ != nullptr) {

                        this->message_callback_(shared_from_this(), parser_);
                    }

                    // 擦除已经获取的数据报文，重置接收下一个包
                    this->input_buffer_.erase(this->input_buffer_.begin(), this->input_buffer_.begin() + parsedLength);
                    parsedLength = 0;
                }
            // }
        }

        void TcpConnection::handleWrite() {
            if (!output_buffer_.empty()) {
                ssize_t n = ::write(this->socket_->getFd(), this->output_buffer_.data(), this->output_buffer_.size());
                if (n > 0) {
                    // 删掉已经成功写入的数据
                    this->output_buffer_.erase(this->output_buffer_.begin(), this->output_buffer_.begin() + n);

                    if (this->output_buffer_.empty()) {
                        // 防止CPU空转
                        this->channel_->disableWriting();

                        if (this->state_ == ConnectionState::DISCONNECTED) {
                            ::shutdown(this->socket_->getFd(), SHUT_WR); // 数据发送完毕，关闭套接字
                        }
                    }
                } else {
                    std::cerr << "...";
                }
            }
        }

        void TcpConnection::handleClose() {
            if (this->state_ == ConnectionState::CLOSED) {
                return;
            }

            this->state_ = ConnectionState::CLOSED;
            this->channel_->disableAll(); // 注销所有事件

            if (this->close_callback_ != nullptr) {
                this->close_callback_(shared_from_this());
            }
        }

        void TcpConnection::handleError() {
            int err = 0;
            socklen_t len = sizeof(err);
            ::getsockopt(this->socket_->getFd(), SOL_SOCKET, SO_ERROR, &err, &len);
            std::cerr << "...";
        }
    }
} // mnsx