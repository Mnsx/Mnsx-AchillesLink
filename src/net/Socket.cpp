/** 
 * @file Socket.cpp
 * @author Mnsx_x <xx1527030652@gmail.com>
 * @date 2026/4/8
 * @description 封装原生Socket文件描述符，基于RAII管理生命周期，并且提供非阻塞和套接字选项配置
 */
#include "Socket.h"
#include "Logger.h"

#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <cstring>
#include <cerrno>
#include <system_error>
#include <memory>
#include <iostream>

using namespace mnsx::achilles;

Socket::Socket() {
    // 创建socket
    fd_ = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd_ < 0) {
        LOG_ERROR << "Failed to create socket! Reason: " << strerror(errno);
    }
}

Socket::~Socket() {
    // 判断当前socket的fd是否合法
    if (isValid()) {
        ::close(fd_);
    }
}

Socket &Socket::operator=(Socket && other) noexcept {
    // 自我检测
    if (this != &other) {
        if (isValid()) {
            ::close(fd_);
        }
        // 移动other
        fd_ = other.fd_;
        other.fd_ = -1;
    }

    return *this;
}

bool Socket::bind(const InetAddress &address) {
    int ret = ::bind(fd_, address.getAddr(), sizeof(struct sockaddr));
    if (ret != 0) {
        LOG_WARN << "Socket bind to " << address.getIp()
                 << ":" << address.getPort() << " failed! Reason: " << strerror(errno);
    }
    return ret == 0;
}

bool Socket::connect(const InetAddress &address) {
    return ::connect(fd_, address.getAddr(), sizeof(struct sockaddr)) == 0;
}

bool Socket::listen(int backlog) {
    return ::listen(fd_, backlog) == 0;
}

int Socket::accept(InetAddress &peerAddress) {
    sockaddr_in temp{};
    socklen_t len = sizeof(temp);

    int new_fd = ::accept(fd_, reinterpret_cast<sockaddr *>(&temp), &len);

    if (new_fd < 0) {
        // 在非阻塞模式下，EAGAIN/EWOULDBLOCK是正常的，表示暂时没有新连接
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            LOG_ERROR << "Socket accept error! Reason: " << strerror(errno);
        }
    } else {
        peerAddress.setAddr(temp);
        LOG_DEBUG << "Accepted new connection from "
                  << peerAddress.getIp() << ":" << peerAddress.getPort() << " [FD: " << new_fd << "]";
    }

    return new_fd;
}

void Socket::setNonBlocking(bool on) {
    // 获取socket的所有设置
    int flags = ::fcntl(fd_, F_GETFL, 0);
    if (on) {
        flags |= O_NONBLOCK;
    } else {
        flags &= ~O_NONBLOCK;
    }
    // 设置socket
    ::fcntl(fd_, F_SETFL, flags);
}

void Socket::setReuseAddr(bool on) {
    int opt = on ? 1 : 0;
    ::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
}

void Socket::setReusePort(bool on) {
    int opt = on ? 1 : 0;
    ::setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
}

// TODO 修改
std::unique_ptr<Socket> Socket::createServerSocket(uint16_t port) {
    // 创建一个非阻塞的Socket
    int sock_fd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (sock_fd < 0) {
        LOG_ERROR << "Server socket creation failed! Reason: " << strerror(errno);
        exit(EXIT_FAILURE);
    }

    // 将fd放入Socket中，由Socket接管他的声明周期
    auto socket = std::unique_ptr<Socket>(new Socket(sock_fd));

    // 配置
    socket->setReuseAddr(true);
    socket->setReusePort(true);
    int res = socket->bind(InetAddress(port));

    return socket;
}