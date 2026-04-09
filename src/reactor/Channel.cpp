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

        Channel::Channel(EventLoop* loop, int fd) : loop_(loop), fd_(fd), events_(NONE_EVENT), revents_(0) {}

        void Channel::handleEvent() {

            // 断开事件
            if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
                if (closeCallback_) closeCallback_();
            }
            // 错误事件
            if (revents_ & EPOLLERR) {
                if (errorCallback_) errorCallback_();
            }
            // 处理可读事件
            if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
                if (readCallback_) readCallback_();
            }
            // 处理可写事件
            if (revents_ & EPOLLOUT) {
                if (writeCallback_) writeCallback_();
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
            this->loop_->updateChannel(this);
        }
    }
}
