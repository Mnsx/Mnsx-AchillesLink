/** 
 * @file Epoll.h
 * @author Mnsx_x <xx1527030652@gmail.com>
 * @date 2026/4/8
 * @description 
 */
#ifndef MNSX_ACHILLESLINK_EPOLL_H
#define MNSX_ACHILLESLINK_EPOLL_H


#include <sys/epoll.h>
#include <vector>
#include <memory>

namespace mnsx {
    namespace achilles {

        class Epoll {
        public:
            /**
             * @brief 构造函数
             */
            Epoll();

            /**
             * @brief 析构函数
             */
            ~Epoll();

            // 禁止拷贝
            Epoll(const Epoll&) = delete;
            Epoll& operator=(const Epoll&) = delete;

            /**
             * @brief 更新/添加监控事件
             * @param fd 文件描述符
             * @param events 关心的事件
             */
            void updateEvent(int fd, uint32_t events);

            /**
             * @brief 移除监控事件
             * @param fd 需要移除的文件描述符
             */
            void removeEvent(int fd);

            /**
             * @brief 等待事件发生
             * @param timeout_ms 超时事件（毫秒），-1为永久等待
             * @return 激活的事件列表
             */
            std::vector<struct epoll_event> poll(int  timeout_ms = -1);

        private:
            int epoll_fd_; // epoll文件描述符
            std::vector<struct epoll_event> events_; // 存储活跃事件的缓冲区
            constexpr const static int MAX_EVENTS = 10000; // 最大监控事件数
        };
    }
}


#endif //MNSX_ACHILLESLINK_EPOLL_H