/** 
 * @file Channel.cpp
 * @author Mnsx_x <xx1527030652@gmail.com>
 * @date 2026/4/8
 * @description 
 */
#include "Channel.h"

#include "Epoll.h"
#include "EventLoop.h"

namespace mnsx {
    namespace achilles {

        // 定义Epoll事件标志
        constexpr const uint32_t NONE_EVENT = 0;
        constexpr const uint32_t READ_EVENT = EPOLLIN | EPOLLPRI | EPOLLRDHUP;
        constexpr const uint32_t WRITE_EVENT = EPOLLOUT | EPOLLET;

        Channel::Channel(EventLoop *loop, int fd) : loop_(loop), fd_(fd), events_(NONE_EVENT), active_events_(0) {}

        void Channel::handleEvent() {
            // 断开事件
            if ((active_events_ & EPOLLHUP) && !(active_events_ & EPOLLIN)) {
                if (close_callback_) close_callback_();
            }
            // 错误事件
            if (active_events_ & EPOLLERR) {
                if (error_callback_) error_callback_();
            }
            // 处理可读事件
            if (active_events_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
                if (read_callback_) read_callback_();
            }
            // 处理可写事件
            if (active_events_ & EPOLLOUT) {
                if (write_callback_) write_callback_();
            }
        }

        void Channel::enableReading() {
            events_ |= READ_EVENT;
            update();
        }

        void Channel::enableWriting() {
            events_ |= WRITE_EVENT;
            update();
        }

        void Channel::disableWriting() {
            events_ &= ~WRITE_EVENT;
            update();
        }

        void Channel::disableAll() {
            events_ = NONE_EVENT;
            update();
        }

        void Channel::update() {
            loop_->updateChannel(this);
        }

    }
}
