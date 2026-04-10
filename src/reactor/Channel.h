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
            using EventCallback = std::function<void()>; // 回调函数类型，业务层可以通过回调函数设置针对不同的事件的handle

            /**
             * 构造函数
             * @param loop
             * @param fd
             */
            Channel(EventLoop* loop, int fd);
            /**
             * 析构函数
             */
            ~Channel() = default;

            /**
             * 核心，分发任务，根据active_events_去执行handle，即将epoll和handle绑定
             */
            void handleEvent();

            /**
             * 允许通道的读行为
             */
            void enableReading();
            /**
             * 允许通道的写行为
             */
            void enableWriting();
            /**
             * 禁止通道的写行为
             */
            void disableWriting();
            /**
             * 禁止通道的读写行为
             */
            void disableAll();

            /**
             * read_callback_ Setter
             * @param cb
             */
            void setReadCallback(EventCallback cb) { read_callback_ = std::move(cb); }
            /**
             * write_callback_ Setter
             * @param cb
             */
            void setWriteCallback(EventCallback cb) { write_callback_ = std::move(cb); }
            /**
             * close_callback_ Setter
             * @param cb
             */
            void setCloseCallback(EventCallback cb) { close_callback_ = std::move(cb); }
            /**
             * error_callback_ Setter
             * @param cb
             */
            void setErrorCallback(EventCallback cb) { error_callback_ = std::move(cb); }
            /**
             * active_events_ Setter
             * @param active_events
             */
            void setActiveEvents(uint32_t active_events) { active_events_ = active_events; }
            /**
             * fd_ Getter
             * @return
             */
            int getFd() const { return fd_; }
            /**
             * events_ Getter
             * @return
             */
            uint32_t getEvents() const { return events_; }
            /**
             * active_events_ Getter
             * @return
             */
            uint32_t getActiveEvents() const { return active_events_; }

        private:
            /**
             * 向epoll更新当前通道关注的事件
             */
            void update();

            EventLoop* loop_; // 管理这个Channel的EventLoop
            int fd_; // 这个通道对应的fd
            uint32_t events_; // 订阅的事件
            uint32_t active_events_; // 实际发生的事件

            // 针对不同事件的回调函数，相当于模型的handle
            EventCallback read_callback_;
            EventCallback write_callback_;
            EventCallback close_callback_;
            EventCallback error_callback_;
        };

    }
} // mnsx

#endif //MNSX_ACHILLESLINK_CHANNEL_H