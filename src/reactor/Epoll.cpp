/** 
 * @file Epoll.cpp
 * @author Mnsx_x <xx1527030652@gmail.com>
 * @date 2026/4/8
 * @description 
 */
#include "Epoll.h"

#include <stdexcept>
#include <unistd.h>

namespace mnsx {
    namespace achilles {
        Epoll::Epoll() : events_(MAX_EVENTS) {
            // 保证进程执行exec时关闭次fd
            this->epoll_fd_ = ::epoll_create1(EPOLL_CLOEXEC);
            if (this->epoll_fd_ == -1) {
                throw std::runtime_error("[Mnsx-Achilles] epoll_create1 failed...");
            }
        }

        Epoll::~Epoll() {
            if (this->epoll_fd_ != -1) {
                ::close(this->epoll_fd_);
            }
        }

        void Epoll::updateEvent(int fd, uint32_t events) {
            // 封装结构体
            struct epoll_event ee{};
            ee.data.fd = fd;
            ee.events = events;

            // 将事件添加到监管红黑树中，如果已经存在则使用MOD
            if (::epoll_ctl(this->epoll_fd_, EPOLL_CTL_ADD, fd, &ee) == -1) {
                if (errno == EEXIST) {
                    ::epoll_ctl(this->epoll_fd_, EPOLL_CTL_MOD, fd, &ee);
                }
            }
        }

        void Epoll::removeEvent(int fd) {
            ::epoll_ctl(this->epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
        }

        std::vector<struct epoll_event> Epoll::poll(int timeout_ms) {
            // 等待事件发生，并将关注事件存入列表中
            int event_count = ::epoll_wait(this->epoll_fd_, this->events_.data(),
                static_cast<int>(this->events_.size()), timeout_ms);

            std::vector<struct epoll_event> active_events;
            for (int i = 0; i < event_count; ++i) {
                active_events.push_back(this->events_[i]);
            }
            return active_events;
        }
    }
}
