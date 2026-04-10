/** 
 * @file EventLoop.h
 * @author Mnsx_x <xx1527030652@gmail.com>
 * @date 2026/4/9
 * @description 封装事件循环、线程标识以及跨线程任务调度
 */
#ifndef MNSX_ACHILLESLINK_EVENTLOOP_H
#define MNSX_ACHILLESLINK_EVENTLOOP_H

#include <functional>
#include <vector>
#include <mutex>
#include <memory>
#include <atomic>
#include <unistd.h>
#include <sys/syscall.h>

namespace mnsx {
    namespace achilles {

        class Epoll;
        class Channel;
        class EventLoop {
        public:
            using Functor = std::function<void()>; // 其他线程投放的任务

            /**
             * 构造函数
             */
            EventLoop();

            /**
             * 析构函数
             */
            ~EventLoop();

            /**
             * 开启事件循环，阻塞等待epoll中有发生的事件
             */
            void loop();

            /**
             * 推出循环
             */
            void quit();

            /**
             * 在Loop线程中运行闭包
             * @param functor
             */
            void runInLoop(Functor functor);

            /**
             * 将闭包加入到任务队列中
             * @param functor
             */
            void enqueueInLoop(Functor functor);

            /**
             * 判断当前线程是不是Loop线程
             * @return
             */
            bool isInLoopThread() const;

            /**
             * 更新通道关注的事件
             * @param channel
             */
            void updateChannel(Channel* channel);

            /**
             * 删除通道
             * @param channel
             */
            void removeChannel(Channel* channel);

        private:
            /**
             * 发送唤醒epoll的心跳包，因为底层epoll一直很在wait，所以发送一个事件唤醒。来处理其他线程的任务
             */
            void wakeUp();
            /**
             * 处理心跳包，如果不处理会在缓冲区，污染其他消息
             */
            void handleWakeUp();

            /**
             * 执行任务队列中其他线程的任务，因为TcpConnection不是线程安全的，所以需要将其他线程的操作丢给Loop线程完成
             */
            void doPendingFunctors();

            std::atomic<bool> looping_; // 同下，用来判断是否开始循环
            std::atomic<bool> quit_; // 因为会有多线程向这个线程投递任务，所以使用原子变量，判断是否已经退出

            const pid_t thread_id_; // 当前线程的id

            std::unique_ptr<Epoll> epoll_; // EventLoop的核心epoll

            int wake_up_fd_; // 用来处理心跳包的Socket的fd
            std::unique_ptr<Channel> wake_up_channel_; // 专门用于处理心跳包的Channel

            std::mutex mutex_; // 保护任务队列的互斥锁
            std::vector<Functor> pending_functors_; // 存放其他线程投递的任务
            std::atomic<bool> calling_pending_functors_; // 当前是否正在执行其他线程的任务
        };

        inline bool EventLoop::isInLoopThread() const {
            return thread_id_ == ::syscall(SYS_gettid);
        }

    }
}

#endif //MNSX_ACHILLESLINK_EVENTLOOP_H