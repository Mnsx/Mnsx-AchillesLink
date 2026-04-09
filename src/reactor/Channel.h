/** 
 * @file Channel.h
 * @author Mnsx_x <xx1527030652@gmail.com>
 * @date 2026/4/8
 * @description 核心事件分离器，绑定特定文件描述符及读写事件回调，衔接底层Epoll和上层业务
 */
#ifndef MNSX_ACHILLESLINK_CHANNEL_H
#define MNSX_ACHILLESLINK_CHANNEL_H

#include <functional>
#include <cstdint>
#include <sys/epoll.h>

namespace mnsx {
    namespace achilles {

        class EventLoop;
        class Channel {
        public:
            // 回调函数，业务层解耦
            using EventCallback = std::function<void()>;

            /**
             * @brief 构造函数
             */
            Channel(EventLoop* loop, int fd);
            /**
             * @brief 析构函数
             */
            ~Channel() = default;

            /**
             * @brief 核心功能，分发事件
             */
            void handleEvent();

            /**
             * @brief 状态允许读
             */
            void enableReading();
            /**
             * @brief 状态允许写
             */
            void enableWriting();
            /**
             * @brief 状态禁止写
             */
            void disableWriting();
            /**
             * @brief 状态禁止读写
             */
            void disableAll();

            /**
             * @brief Setter
             * @param cb
             */
            void setReadCallback(EventCallback cb) { this->readCallback_ = cb; }
            /**
             * @brief Setter
             * @param cb
             */
            void setWriteCallback(EventCallback cb) { this->writeCallback_ = cb; }
            /**
             * @brief Setter
             * @param cb
             */
            void setCloseCallback(EventCallback cb) { this->closeCallback_ = cb; }
            /**
             * @brief Setter
             * @param cb
             */
            void setErrorCallback(EventCallback cb) { this->errorCallback_ = cb; }
            /**
             * @brief Setter
             * @param events
             */
            void setRevents(uint32_t events) {this->revents_ = events;}

            /**
             * @brief Getter
             * @return
             */
            int getFd() const { return fd_; }
            /**
             * @brief Getter
             * @return
             */
            uint32_t getEvents() const { return events_; }
        private:
            /**
             * @brief 更新当前通道的状态
             */
            void update();

            EventLoop* loop_; // 所属的eventLoop
            int fd_; // 监控的句柄
            uint32_t events_; // 订阅的事件
            uint32_t revents_; // 实际发生的事件

            // 回调函数
            EventCallback readCallback_;
            EventCallback writeCallback_;
            EventCallback closeCallback_;
            EventCallback errorCallback_;
        };
    }
} // mnsx

#endif //MNSX_ACHILLESLINK_CHANNEL_H