/** 
 * @file Socket.h
 * @author Mnsx_x <xx1527030652@gmail.com>
 * @date 2026/4/8
 * @description 封装Linux原生Socket
 */
#ifndef MNSX_ACHILLESLINK_SOCKET_H
#define MNSX_ACHILLESLINK_SOCKET_H


#include "InetAddress.h"

namespace mnsx {
    namespace achilles {

        class Socket {
        public:
            /**
             * @brief 默认构造，创建Socket
             */
            Socket();

            /**
             * @brief 包装一个已有的文件描述符
             * @param fd 文件描述符
             */
            explicit Socket(int fd) : fd_(fd) {}

            /**
             * @brief 析构函数
             */
            ~Socket();

            // 禁止拷贝，防止Socket被多次关闭
            Socket(const Socket&) = delete;
            Socket& operator=(const Socket&) = delete;

            // 允许移动操作
            Socket(Socket&&) noexcept;
            Socket& operator=(Socket&&) noexcept;

            /**
             * @brief Socket绑定
             * @param addr 绑定的网络地址
             * @return 是否成功
             */
            bool bind(const InetAddress& addr);

            /**
             * @brief 监听Socket
             * @param backlog 等待accept的Socket的个数
             * @return 是否成功
             */
            bool listen(int backlog = 1024);

            /**
             * @brief 将等待的Socket指向目标地址
             * @param peerAddr 目标地址
             * @return 返回跟这个地址通话的文件描述符句柄
             */
            int accept(InetAddress& peerAddr);

            // 配置
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

            // Getter
            int getFd() const {
                return this->fd_;
            }

            /**
             * @brief 判断当前句柄是否合法
             * @return 是否合法
             */
            bool isValid() const {
                return fd_ != -1;
            }

        private:
            int fd_; // 套接字的句柄
        };
    }
}


#endif //MNSX_ACHILLESLINK_SOCKET_H