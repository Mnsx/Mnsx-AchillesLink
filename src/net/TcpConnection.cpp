/**
 * @file TcpConnection.cpp
 * @author Mnsx_x <xx1527030652@gmail.com>
 * @date 2026/4/8
 * @description 核心连接抽象，负责单条TCP连接的状态机、缓冲区以及非阻塞I/O数据收发
 */
#include "../../include/net/TcpConnection.h"
#include "../../include/reactor/EventLoop.h"
#include "Logger.h"

#include <unistd.h>
#include <sys/socket.h>
#include <cerrno>
#include <iostream>
#include <cstring>

using namespace mnsx::achilles;

TcpConnection::TcpConnection(EventLoop *loop, int conn_fd) : loop_(loop), state_(ConnectionState::CONNECTING),
    socket_(new Socket(conn_fd)), channel_(new Channel(loop, conn_fd)) {

    // 将Socket设置为非阻塞模式
    socket_->setNonBlocking(true);
    // 将处理处理事件的函数添加到Channel回调中欧给
    this->channel_->setReadCallback([this]() { this->handleRead(); });
    this->channel_->setWriteCallback([this]() { this->handleWrite(); });
    this->channel_->setCloseCallback([this]() { this->handleClose(); });
    this->channel_->setErrorCallback([this]() { this->handleError(); });

    LOG_DEBUG << "TcpConnection [FD: " << conn_fd << "] constructed.";
}

TcpConnection::~TcpConnection() {
    LOG_INFO << "TcpConnection [FD: " << socket_->getFd() << "] cleanly destroyed.";
}

void TcpConnection::connectEstablished() {
    // 修改状态机
    this->state_ = ConnectionState::CONNECTED;
    // 监听读事件
    this->channel_->enableReading();

    LOG_INFO << "TcpConnection [FD: " << socket_->getFd() << "] established and ready.";

    // 执行连接事件
    if (this->on_connection_callback_ != nullptr) {
        this->on_connection_callback_(shared_from_this());
    }
}

void TcpConnection::send(const std::string& message) {
    if (this->state_ != ConnectionState::CONNECTED) return;

    if (this->loop_->isInLoopThread()) {
        sendInLoop(message.data(), message.size());
    } else {
        auto self = shared_from_this();
        // 捕获 message 的副本 (按值捕获 std::string)
        this->loop_->runInLoop([self, message]() {
            self->sendInLoop(message.data(), message.size());
        });
    }
}

void TcpConnection::send(const char* data, size_t len) {
    // 判断状态机，如果连接已经关闭，不允许发送
    if (this->state_ != ConnectionState::CONNECTED) {
        return;
    }
    // 判断是否在Loop线程的任务
    if (this->loop_->isInLoopThread()) {

        sendInLoop(data, len);
    } else {
        // 如果不是Loop线程，需要将自己的this，发给Loop线程，这就属于异步，必须使用share_from_this()
        auto self = shared_from_this();
        this->loop_->runInLoop([self, data, len]() {
            self->sendInLoop(data, len);
        });
    }
}

void TcpConnection::shutdown() {
    // 判断状态机
    if (state_ == ConnectionState::CONNECTED) {
        state_ = ConnectionState::DISCONNECTED;

        // 如果发送缓冲区还没有关闭，那么需要等待写事件处理后关闭
        if (this->output_buffer_.readableBytes() == 0) {
            // 所以这里使用半关闭，允许继续发送，将真正的关闭放在发送事件处理的最后
            ::shutdown(this->socket_->getFd(), SHUT_WR);
            LOG_DEBUG << "TcpConnection [FD: " << socket_->getFd() << "] half-closed (SHUT_WR).";
        }
    }
}

void TcpConnection::sendInLoop(const char* data, size_t len) {
    // 判断状态机
    if (state_ != ConnectionState::CONNECTED) {
        LOG_ERROR << "Disconnected, give up writing";
        return;
    }

    ssize_t write_size = 0;
    size_t remaining = len;
    bool faultError = false;

    // 如果缓冲区没有内容，说明消息没有拥挤，直接通过原生代码发送
    if (output_buffer_.readableBytes() == 0) {
        write_size = ::write(socket_->getFd(), data, len);
        if (write_size >= 0) {
            remaining -= write_size;
        } else {
            write_size = 0;
            // 因为是非阻塞模式，所以内核缓冲区被写满后只会返回EAGAIN
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                LOG_ERROR << "TcpConnection [FD: " << socket_->getFd()
                                  << "] write error: " << strerror(errno);
                faultError = true;
            }
        }
    }

    // 如果是内核被装满了，那么就把剩下的装进output_buffer_中处理
    if (!faultError && remaining > 0) {
        LOG_DEBUG << "TcpConnection [FD: " << socket_->getFd()
                          << "] kernel buffer full, appending " << remaining << " bytes to app buffer.";

        output_buffer_.append(data + write_size, remaining);
        // 开启写事件监听
        channel_->enableWriting();
    }
}

void TcpConnection::handleRead() {
    // 因为有ByteBuffer所以不需要像使用vector一样全部读完
    int saveErrno = 0;
    // 将数据添加到input_buffer_中
    ssize_t input_count = input_buffer_.readFd(channel_->getFd(), &saveErrno);

    if (input_count > 0) {
        // 将数据处理与连接解耦，让数据解析器来处理
        if (on_modbus_message_callback_) {
            on_modbus_message_callback_(shared_from_this(), &input_buffer_);
        }
    } else if (input_count == 0) {
        // 读到 0，说明对端优雅地关闭了连接
        LOG_INFO << "TcpConnection::handleRead - Client closed connection, fd: " << channel_->getFd();
        handleClose();
    } else {
        // 读取出错
        errno = saveErrno;
        LOG_ERROR << "TcpConnection::handleRead error, fd: " << channel_->getFd() << " errno: " << errno;
        handleError();
    }
}

void TcpConnection::handleWrite() {
    // 如果output_buffer_不为空
    if (output_buffer_.readableBytes() > 0) {
        ssize_t write_count = ::write(this->socket_->getFd(), this->output_buffer_.peek(), this->output_buffer_.readableBytes());
        if (write_count > 0) {
            // 消耗数据
            output_buffer_.retrieve(write_count);

            if (this->output_buffer_.readableBytes() == 0) {
                // 防止CPU空转，如果已经写完所有数据，那么关闭写事件防止空转
                this->channel_->disableWriting();

                if (this->state_ == ConnectionState::DISCONNECTED) {
                    // 对应shutdown中半关闭，如果状态机已关闭，并且数据写入完毕，那么推出
                    ::shutdown(this->socket_->getFd(), SHUT_WR);
                    LOG_DEBUG << "TcpConnection [FD: " << socket_->getFd() << "] half-closed after flushing buffer.";
                }
            }
        } else {
            LOG_ERROR << "TcpConnection [FD: " << socket_->getFd()
                                              << "] async write error: " << strerror(errno);        }
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
    LOG_ERROR << "TcpConnection [FD: " << this->socket_->getFd()
                      << "] socket error: " << strerror(err);
}

void TcpConnection::setCloseCallback(OnCloseCallback on_close) {
    on_close_callback_ = on_close;
}

void TcpConnection::setMessageCallback(OnMessageCallback on_message) {
    on_modbus_message_callback_ = on_message;
}

void TcpConnection::setConnectionCallback(OnConnectionCallback on_connection) {
    on_connection_callback_ = on_connection;
}