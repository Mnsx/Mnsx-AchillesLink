/** 
 * @file EventLoop.cpp
 * @author Mnsx_x <xx1527030652@gmail.com>
 * @date 2026/4/9
 * @description 封装事件循环、线程标识以及跨线程任务调度
 */
#include "../include/EventLoop.h"

#include <iostream>
#include <sys/eventfd.h>

#include "Channel.h"
#include "Epoll.h"

namespace mnsx {
    namespace achilles {
        // 创建一个eventfd
        int createEventFd() {
            int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
            if (evtfd < 0) {
                std::cerr << "create eventfd failed" << std::endl;
                exit(EXIT_FAILURE);
            }
            return evtfd;
        }

        EventLoop::EventLoop() : looping_(false), quit_(false),
            thread_id_(::syscall(SYS_gettid)), epoll_(new Epoll()),
            wake_up_fd_(createEventFd()), wake_up_channel_(new Channel(this, wake_up_fd_)),
            calling_pending_functors_(false) {

            // 将handleWakeUp绑定在Channel的可读回调中
            wake_up_channel_->setReadCallback([this]() {
                this->handleWakeUp();
            });
            // 开启监听，只要有线程写入数据，就会触发，删除心跳包
            wake_up_channel_->enableReading();
        }

        EventLoop::~EventLoop() {
            wake_up_channel_->disableAll();
            ::close(wake_up_fd_);
        }

        void EventLoop::loop() {
            looping_ = true;
            quit_ = false;

            while (quit_ != true) {
                // 阻塞等待事件发生，epoll返回活跃的Channel
                std::vector<Channel*> active_channels = epoll_->poll();

                // 遍历处理所有事件
                for (auto& channel : active_channels) {
                    channel->handleEvent();
                }

                // 执行其他线程的任务
                doPendingFunctors();
            }

            looping_ = false;
        }

        void EventLoop::quit() {
            quit_ = true;
            // 唤醒Loop线程，检测关闭
            if (isInLoopThread() != true) {
                wakeUp();
            }
        }

        void EventLoop::runInLoop(Functor cb) {
            if (isInLoopThread()) {
                // 本线程的任务直接执行
                cb();
            } else {
                queueInLoop(std::move(cb));
            }
        }

        void EventLoop::queueInLoop(Functor cb) {
            // 加入任务队列
            {
                std::lock_guard<std::mutex> lock(mutex_);
                pending_functors_.push_back(std::move(cb));
            }

            // 如果不是本线程执行需要唤醒，如果正在执行任务，唤醒，因为是将任务队列复制后执行，如果不换形，新加入的任务饿死
            if (isInLoopThread() != true || calling_pending_functors_.load() == true) {
                wakeUp();
            }
        }

        void EventLoop::updateChannel(Channel *channel) {
            this->epoll_->updateEvent(channel);
        }

        void EventLoop::removeChannel(Channel *channel) {
            this->epoll_->removeEvent(channel);
        }

        void EventLoop::wakeUp() {
            uint64_t packet = 1;
            // 发送8字节心跳包
            ssize_t n = ::write(wake_up_fd_, &packet, sizeof(packet));
            if (n != sizeof(packet)) {
                std::cerr << "EventLoop::wakeUp() write error" << std::endl;
            }
        }

        void EventLoop::handleWakeUp() {
            uint64_t packet = 1;
            // 发送8字节心跳包
            ssize_t n = ::read(wake_up_fd_, &packet, sizeof(packet));
            if (n != sizeof(packet)) {
                std::cerr << "EventLoop::wakeUp() write error" << std::endl;
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
    }
}
