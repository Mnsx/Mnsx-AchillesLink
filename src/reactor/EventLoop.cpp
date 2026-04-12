/** 
 * @file EventLoop.cpp
 * @author Mnsx_x <xx1527030652@gmail.com>
 * @date 2026/4/9
 * @description 封装事件循环、线程标识以及跨线程任务调度
 */
#include "../../include/reactor/EventLoop.h"
#include "Channel.h"
#include "Epoll.h"
#include "Logger.h"

#include <iostream>
#include <sys/eventfd.h>
#include <cstring>

using namespace mnsx::achilles;

/**
 * 辅助函数创建一个负责唤醒的管理者
 * @return
 */
int createEventFd() {
   int event_fd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
   if (event_fd < 0) {
       LOG_ERROR << "Failed to create eventfd! Reason: " << strerror(errno);
       exit(EXIT_FAILURE);
   }
   return event_fd;
}

EventLoop::EventLoop() : looping_(false), quit_(false), thread_id_(::syscall(SYS_gettid)),
    epoll_(new Epoll), wake_up_fd_(createEventFd()), wake_up_channel_(new Channel(this, wake_up_fd_)),
        calling_pending_functors_(false) {

    LOG_DEBUG << "EventLoop created. Bound to Thread ID: " << thread_id_ << ", WakeUp FD: " << wake_up_fd_;

    wake_up_channel_->setReadCallback([this]() {
        // 读取到心跳包，立马处理
        this->handleWakeUp();
    });
    // 开启监听，允许读事件，读到心跳包，执行回调处理心跳包
    wake_up_channel_->enableReading();
}

EventLoop::~EventLoop() {
    LOG_DEBUG << "EventLoop in Thread ID: " << thread_id_ << " is being destroyed.";
    wake_up_channel_->disableAll();
    ::close(wake_up_fd_);
}

void EventLoop::loop() {
    looping_.store(true);
    quit_.store(false);

    LOG_INFO << "EventLoop starting to poll in Thread ID: " << thread_id_;

    while (quit_.load() != true) {
        // 阻塞等待，epoll返回触发的事件Channel
        std::vector<Channel*> active_channels = epoll_->poll();
        // 便利所有的触发事件，并执行对应的handle
        for (auto& channel : active_channels) {
            channel->handleEvent();
        }
        // 执行其他线程的任务
        doPendingFunctors();
    }

    // 跳出循环
    LOG_INFO << "EventLoop stopped polling in Thread ID: " << thread_id_;
    looping_.store(false);
}

void EventLoop::quit() {
    // 修改原子量
    quit_.store(true);
    LOG_DEBUG << "EventLoop quit signal triggered. Target Thread ID: " << thread_id_;

    // 唤醒Loop线程，检测
    if (isInLoopThread() != true) {
        // 如果不是当前线程，就唤醒Loop，让Loop识别到quit_变化
        wakeUp();
    }
}

void EventLoop::runInLoop(Functor functor) {
    if (isInLoopThread()) {
        // 如果是本线程的任务，直接执行
        functor();
    } else {
        // 如果不是本线程的任务，加入任务队列，等待执行
        enqueueInLoop(std::move(functor));
    }
}

void EventLoop::enqueueInLoop(Functor functor) {
    // 使用互斥锁，保证加入任务队列时线程安全
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pending_functors_.push_back(std::move(functor));
    }

    // 如果不是本线程执行需要唤醒，如果正在执行任务，唤醒，因为是将任务队列复制后执行，如果不换形，新加入的任务饿死
    if (isInLoopThread() != true || calling_pending_functors_.load() == true) {
        wakeUp();
    }
}

void EventLoop::doPendingFunctors() {
    std::vector<Functor> functors;
    calling_pending_functors_.store(true);

    // 性能优化，如果在原来的vector中执行，全程加锁，复制一个新的容器，解锁
    {
        std::lock_guard<std::mutex> lock(mutex_);
        functors.swap(pending_functors_);
    }

    // 执行新的容器中的任务，不会占用锁
    for (auto& cb : functors) {
        cb();
    }

    calling_pending_functors_.store(false);
}

void EventLoop::updateChannel(Channel *channel) {
    this->epoll_->updateEvent(channel);
}

void EventLoop::removeChannel(Channel *channel) {
    this->epoll_->removeEvent(channel);
}

void EventLoop::wakeUp() {

    // 发送8字节的心跳包，epoll_fd就是一个8字节的计数器
    uint64_t packet = 1;
    // 发送8字节心跳包
    ssize_t n = ::write(wake_up_fd_, &packet, sizeof(packet));
    if (n != sizeof(packet)) {
        LOG_ERROR << "EventLoop::wakeUp() write error! Reason: " << strerror(errno);
    }
}

void EventLoop::handleWakeUp() {
    uint64_t packet = 1;
    // 发送8字节心跳包
    ssize_t n = ::read(wake_up_fd_, &packet, sizeof(packet));
    if (n != sizeof(packet)) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            LOG_ERROR << "EventLoop::handleWakeUp() read error! Reason: " << strerror(errno);
        }
    }
}
