/** 
 * @file Socket.h
 * @author Mnsx_x <xx1527030652@gmail.com>
 * @date 2026/4/8
 * @description 封装原生Socket文件描述符，基于RAII管理生命周期，并且提供非阻塞和套接字选项配置
 */
#ifndef MNSX_ACHILLESLINK_SOCKET_H
#define MNSX_ACHILLESLINK_SOCKET_H

#include "InetAddress.h"

#include <memory>

namespace mnsx {
    namespace achilles {

        class Socket {
        public:
            /**
             * 空参构造函数
             */
            Socket();
            /**
             * 包装一个已经存在的fd
             * @param fd
             */
            Socket(int fd) : fd_(fd) {}
            /**
             * 析构函数
             */
            ~Socket();
            /**
             * @delete
             */
            Socket(const Socket&) = delete;
            /**
             * @delete
             */
            Socket& operator=(const Socket&) = delete;
            /**
             * 移动构造函数
             */
            Socket(Socket&& other) noexcept : fd_(other.fd_) { other.fd_ = -1; }
            /**
             * 移动赋值运算符
             * @return
             */
            Socket& operator=(Socket&&) noexcept;

            /**
             * 为socket绑定地址
             * @param address
             * @return
             */
            bool bind(const InetAddress& address);
            /**
             * 监听
             * @param backlog 最大监听个数，默认1024
             * @return
             */
            bool listen(int backlog = 1024);
            /**
             * 为通过握手的socket分配一个新的fd
             * @param peerAddress
             * @return
             */
            int accept(InetAddress& peerAddress);
            /**
             * 连接
             * @param address
             * @return
             */
            bool connect(const InetAddress& address);

            /**
             * 配置，必备，设置socket非阻塞IO
             */
            void setNonBlocking(bool on);
            /**
             * 配置，当进程处于time_wait时，直接退出
             */
            void setReuseAddr(bool on);
            /**
             * 配置，允许一个线程管理多个socket
             */
            void setReusePort(bool on);

            /**
             * 判断文件描述符是否合法
             * @return
             */
            bool isValid() const;

            /**
             * fd_ Getter
             * @return
             */
            int getFd() const { return this->fd_; }

            /**
             * 静态工具函数，创建一个服务端的Socket
             * @param port
             * @return
             */
            static std::unique_ptr<Socket> createServerSocket(uint16_t port);

            /**
             * 静态工具函数，创建一个客户端的Socket
             * @param port
             * @return
             */
            static std::unique_ptr<Socket> createClientSocket(uint16_t port);

        private:
            int fd_; // socket对应的文件标识符
        };

        inline bool Socket::isValid() const {
            return fd_ > 0;
        }

    }
}

#endif //MNSX_ACHILLESLINK_SOCKET_H