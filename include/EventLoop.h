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
            // 其他线程需要执行的任务，即返回计算数据
            using Functor = std::function<void()>;

            /**
             * @brief 构造函数
             */
            EventLoop();

            /**
             * @brief 析构函数
             */
            ~EventLoop();

            /**
             * @brief 开始事件循环
             */
            void loop();

            /**
             * @brief 退出事件循环
             */
            void quit();

            /**
             * @brief 提供给其他线程，将任务传递给Loop的线程执行
             * @param cb
             */
            void runInLoop(Functor cb);

            /**
             * @brief 将任务放到任务队列，并唤醒当前IO线程
             * @param cb
             */
            void queueInLoop(Functor cb);

            /**
             * @brief 判断是否在EventLoop线程中运行
             * @return
             */
            bool isInLoopThread() const;

            /**
             * @brief 更新Channel监听事件
             * @param channel
             */
            void updateChannel(Channel* channel);

            /**
             * @brief 删除监听事件
             * @param channel
             */
            void removeChannel(Channel* channel);

        private:
            /**
             * @brief 唤醒EventLoop
             */
            void wakeUp();

            /**
             * @brief 将唤醒EventLoop的心跳包读取，防止数据干扰
             */
            void handleWakeUp();

            /**
             * @brief 处理队列中其他线程的任务
             */
            void doPendingFunctors();

            std::atomic<bool> looping_; // 原子标识，是否开启循环
            std::atomic<bool> quit_; // 原子标识，是否已经退出

            const pid_t thread_id_; // 当前EventLoop运行的线程

            std::unique_ptr<Epoll> epoll_; // 包装Epoll的智能指针

            int wake_up_fd_; // 用于唤醒的fd
            std::unique_ptr<Channel> wake_up_channel_; // 用于唤醒的Cahnnel

            std::mutex mutex_; // 保护任务队列的互斥锁
            std::vector<Functor> pending_functors_; // 任务队列
            std::atomic<bool> calling_pending_functors_; // 原子标识
        };

        inline bool EventLoop::isInLoopThread() const {
            return thread_id_ == ::syscall(SYS_gettid);
        }
    }
}

#endif //MNSX_ACHILLESLINK_EVENTLOOP_H