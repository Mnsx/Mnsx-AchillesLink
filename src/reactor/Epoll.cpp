/** 
 * @file Epoll.cpp
 * @author Mnsx_x <xx1527030652@gmail.com>
 * @date 2026/4/8
 * @description 封装Linux epoll系统调用，负责高并发场景下的I/O多路复用与底层事件监听
 */
#include "Epoll.h"

#include <stdexcept>
#include <unistd.h>

#include "Channel.h"

namespace mnsx {
    namespace achilles {
        Epoll::Epoll() : events_(MAX_EVENTS) {
            // 保证进程执行exec时关闭次fd
            this->epoll_fd_ = ::epoll_create1(EPOLL_CLOEXEC);
            if (this->epoll_fd_ == -1) {
                // TODO 统一日志格式
                throw std::runtime_error("[Mnsx-Achilles] epoll_create1 failed...");
            }
        }

        Epoll::~Epoll() {
            if (this->epoll_fd_ != -1) {
                ::close(this->epoll_fd_);
            }
        }

        void Epoll::updateEvent(Channel* channel) {
            // 封装结构体
            struct epoll_event event{};
            event.events = channel->getEvents();
            // 将Channel存入data.ptr
            event.data.ptr = channel;

            // 将事件添加到监管红黑树中，如果已经存在则使用MOD
            if (::epoll_ctl(this->epoll_fd_, EPOLL_CTL_ADD, channel->getFd(), &event) == -1) {
                // 防止重复添加
                if (errno == EEXIST) {
                    ::epoll_ctl(this->epoll_fd_, EPOLL_CTL_MOD, channel->getFd(), &event);
                }
            }
        }

        void Epoll::removeEvent(Channel* channel) {
            ::epoll_ctl(this->epoll_fd_, EPOLL_CTL_DEL, channel->getFd(), nullptr);
        }

        std::vector<Channel *> Epoll::poll(int timeout_ms) {

            std::vector<Channel *> active_channels;

            // 等待事件发生，并将关注事件存入列表中
            int event_count = ::epoll_wait(this->epoll_fd_, events_.data(),
                static_cast<int>(events_.size()), timeout_ms);

            // 不能使用移动，因为后续还需要使用
            for (int i = 0; i < event_count; ++i) {
                // 将void*转换为Channel*
                Channel* channel = static_cast<Channel*>(events_[i].data.ptr);

                channel->setRevents(events_[i].events);

                active_channels.push_back(channel);
            }
            return active_channels;
        }
    }
}
