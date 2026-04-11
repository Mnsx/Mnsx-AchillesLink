/** 
 * @file Epoll.cpp
 * @author Mnsx_x <xx1527030652@gmail.com>
 * @date 2026/4/8
 * @description 封装Linux epoll系统调用，负责高并发场景下的I/O多路复用与底层事件监听
 */
#include "Epoll.h"
#include "Channel.h"
#include "net/InetAddress.h"
#include "Logger.h"

#include <stdexcept>
#include <cstring>
#include <unistd.h>

using namespace mnsx::achilles;

Epoll::Epoll() : events_(MAX_EVENTS) {
    // 当线程执行exec产生子进程时应该关闭子进程的fd，防止同时存在两个fd
    this->epoll_fd_ = ::epoll_create1(EPOLL_CLOEXEC);
    if (this->epoll_fd_ == -1) {
        LOG_ERROR << "epoll_create1 failed! Reason: " << strerror(errno);
        throw std::runtime_error("epoll_create1 failed");
    }
    LOG_DEBUG << "Epoll created successfully. FD: " << this->epoll_fd_;
}

Epoll::~Epoll() {
    if (this->epoll_fd_ != -1) {
        ::close(this->epoll_fd_);
        LOG_DEBUG << "Epoll FD: " << this->epoll_fd_ << " closed.";
    }
}

void Epoll::updateEvent(Channel *channel) {
    struct epoll_event event{};
    event.events = channel->getEvents();
    event.data.ptr = channel; // 将channel存放，后续获取event时方便获取对应的通道

    // 将event添加到epoll管理的红黑树中
    if (::epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, channel->getFd(), &event) == -1) {
        // 如果错误是已经存在，那么就改为修改
        if (errno == EEXIST) {
            if (::epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, channel->getFd(), &event) == -1) {
                LOG_ERROR << "epoll_ctl MOD failed for FD: " << channel->getFd()
                                  << ", Reason: " << strerror(errno);
            }
        } else {
            LOG_ERROR << "epoll_ctl ADD failed for FD: " << channel->getFd()
                              << ", Reason: " << strerror(errno);
        }
    }
}

void Epoll::removeEvent(Channel *channel) {
    if (::epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, channel->getFd(), nullptr) == -1) {
        LOG_ERROR << "epoll_ctl DEL failed for FD: " << channel->getFd()
                          << ", Reason: " << strerror(errno);
    }
}

std::vector<Channel *> Epoll::poll(int timeout_ms) {
    std::vector<Channel *> active_channels;

    int active_count = ::epoll_wait(epoll_fd_, events_.data(),
    static_cast<int>(events_.size()), timeout_ms);

    if (active_count < 0) {
        if (errno != EINTR) {
            LOG_ERROR << "epoll_wait failed! Reason: " << strerror(errno);
        }
        return active_channels; // 发生错误，返回空列表
    }

    for (int i = 0; i < active_count; ++i) {
        // 将data.ptr转换为Channel
        auto channel = static_cast<Channel*>(events_[i].data.ptr);
        // 设置发生的事件
        channel->setActiveEvents(events_[i].events);
        // 将channel存放到容器中
        active_channels.push_back(channel);
    }
    return active_channels;
}
