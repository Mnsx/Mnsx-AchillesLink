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
             * @brief 构造函数
             */
            Socket();

            /**
             * @brief 构造函数，包装已存在的文件描述符
             * @param fd
             */
            explicit Socket(int fd) : fd_(fd) {}

            /**
             * @brief 析构函数
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
             * @brief 移动构造函数
             */
            Socket(Socket&&) noexcept;

            /**
             * @brief 移动赋值运算符重载
             * @return
             */
            Socket& operator=(Socket&&) noexcept;

            /**
             * @brief Socket绑定
             * @param addr 绑定对象地址
             * @return
             */
            bool bind(const InetAddress& addr);

            /**
             * @brief 监听Socket
             * @param backlog 等待accept的Socket的个数
             * @return
             */
            bool listen(int backlog = 1024);

            /**
             * @brief 为完成连接的套接字分配新的fd，方便数据传输
             * @param peerAddr 目标地址
             * @return 返回用于数据传输的新的fd
             */
            int accept(InetAddress& peerAddr);

            /**
             * @brief 开启非阻塞模式，epoll模式下必须开启
             * @param on
             */
            void setNonBlocking(bool on);

            /**
             * @brief 防止，重启时地址已被使用的错误（time_wait直接退出）
             * @param on
             */
            void setReuseAddr(bool on);

            /**
             * @brief 允许一个线程管理多个Socket
             * @param on
             */
            void setReusePort(bool on);

            /**
             * @brief Getter
             * @return
             */
            int getFd() const {
                return this->fd_;
            }

            /**
             * @brief 判断当前句柄是否合法
             * @return
             */
            bool isValid() const;

            static std::unique_ptr<Socket> createNoblockSocket(uint16_t port);

        private:
            int fd_; // 套接字的句柄
        };

        inline bool Socket::isValid() const {
            if (this->fd_ < 0) {
                return false;
            }
            return true;
        }
    }
}


#endif //MNSX_ACHILLESLINK_SOCKET_H