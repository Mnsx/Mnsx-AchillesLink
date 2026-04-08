/** 
 * @file Channel.h
 * @author Mnsx_x <xx1527030652@gmail.com>
 * @date 2026/4/8
 * @description 
 */
#ifndef MNSX_ACHILLESLINK_CHANNEL_H
#define MNSX_ACHILLESLINK_CHANNEL_H

#include <functional>
#include <cstdint>
#include <sys/epoll.h>

namespace mnsx {
    namespace achilles {

        class Epoll;

        class Channel {
        public:
            using EventCallback = std::function<void()>;

            /**
             * @brief 构造函数
             */
            Channel(Epoll* epoll, int fd);

            /**
             * @brief 析构函数
             */
            ~Channel() = default;

            /**
             * @brief 分发事件
             */
            void handleEvent();

            // 状态控制
            void enableReading();
            void enableWriting();
            void disableWriting();
            void disableAll();

            // Setter
            void setReadCallback(EventCallback cb) { this->readCallback_ = cb; }
            void setWriteCallback(EventCallback cb) { this->writeCallback_ = cb; }
            void setCloseCallback(EventCallback cb) { this->closeCallback_ = cb; }
            void setErrorCallback(EventCallback cb) { this->errorCallback_ = cb; }
            void setRevents(uint32_t events) {this->revents_ = events;}

            // Getter
            int getFd() const { return fd_; }
            uint32_t getEvents() const { return events_; }
        private:
            void update();

            Epoll* epoll_; // 所属的Epoll
            int fd_; // 监控的句柄
            uint32_t events_; // 订阅的事件
            uint32_t revents_; // 实际发生的事件

            // 回调函数对象
            EventCallback readCallback_;
            EventCallback writeCallback_;
            EventCallback closeCallback_;
            EventCallback errorCallback_;
        };
    }
} // mnsx

#endif //MNSX_ACHILLESLINK_CHANNEL_H