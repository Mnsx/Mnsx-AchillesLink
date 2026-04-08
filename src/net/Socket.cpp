/** 
 * @file Socket.cpp
 * @author Mnsx_x <xx1527030652@gmail.com>
 * @date 2026/4/8
 * @description 接口实现
 */
#include "Socket.h"

#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <system_error>

namespace mnsx {
    namespace achilles {
        Socket::Socket() {
            // 创建IPv4
            this->fd_ = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        }

        Socket::~Socket() {
            if (isValid()) {
                ::close(fd_);
            }
        }

        Socket::Socket(Socket&& other) noexcept : fd_(other.fd_) {
            other.fd_ = -1; // 移动后，做空
        }

        Socket &Socket::operator=(Socket&& other) noexcept {
            if (this != &other) {
                if (isValid()) {
                    // 释放当前的fd
                    ::close(fd_);
                }
                this->fd_ = other.fd_;
                other.fd_ = -1;
            }
            return *this;
        }

        bool Socket::bind(const InetAddress &addr) {
            return ::bind(this->fd_, addr.getSockAddr(), sizeof(sockaddr_in)) == 0;
        }

        bool Socket::listen(int backlog) {
            return ::listen(this->fd_, backlog) == 0;
        }

        int Socket::accept(InetAddress& peerAddr) {
            sockaddr_in addr{};
            socklen_t len = sizeof(addr);

            // 接收新的连接
            int connFd = ::accept(this->fd_, reinterpret_cast<sockaddr *>(&addr), &len);
            if (connFd >= 0) {
                peerAddr.setSockAddr(addr);
            }
            return connFd;
        }

        void Socket::setNonBlocking(bool on) {
            // 获取Socket的所有设置
            int flags = ::fcntl(this->fd_, F_GETFL, 0);
            if (on) {
                flags |= O_NONBLOCK;
            } else {
                flags &= ~O_NONBLOCK;
            }
            // 设置Socket
            ::fcntl(this->fd_, F_SETFL, flags);
        }

        void Socket::setReuseAddr(bool on) {
            int opt = on ? 1 : 0;
            ::setsockopt(this->fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        }

        void Socket::setReusePort(bool on) {
            int opt = on ? 1 : 0;
            ::setsockopt(this->fd_, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
        }
    }
}