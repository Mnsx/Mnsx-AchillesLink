/** 
 * @file Epoll.h
 * @author Mnsx_x <xx1527030652@gmail.com>
 * @date 2026/4/8
 * @description 封装Linux epoll系统调用，负责高并发场景下的I/O多路复用与底层事件监听
 */
#ifndef MNSX_ACHILLESLINK_EPOLL_H
#define MNSX_ACHILLESLINK_EPOLL_H

#include <vector>
#include <sys/epoll.h>

namespace mnsx {
    namespace achilles {

        class Channel;
        class Epoll {
        public:
            /**
             * 构造函数
             */
            Epoll();
            /**
             * 析构函数
             */
            ~Epoll();
            /**
             * @delete
             */
            Epoll(const Epoll&) = delete;
            /**
             * @delete
             */
            Epoll& operator=(const Epoll&) = delete;

            /**
             * 更新关注的事件通道
             * @param channel 需要将channel放入epoll_data中，所以使用指针最合适
             */
            void updateEvent(Channel* channel);
            /**
             * 删除关注的事件通道
             * @param channel
             */
            void removeEvent(Channel* channel);
            /**
             * 等待事件发生，返回已经发生事件的容器
             * @param timeout_ms 超时 默认-1不设置超时
             * @return
             */
            std::vector<Channel *> poll(int  timeout_ms = -1);

        private:
            int epoll_fd_; // epoll文件描述符
            std::vector<struct epoll_event> events_; // 存储活跃事件的缓冲区
            constexpr const static int MAX_EVENTS = 10000; // 最大监控事件数
        };

    }
}

#endif //MNSX_ACHILLESLINK_EPOLL_H