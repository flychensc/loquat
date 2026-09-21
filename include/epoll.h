#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include <sys/epoll.h>

#include "pollable.h"

namespace loquat
{
    class Epoll
    {
    public:
        static constexpr int kMaxEvents = 20;

        static std::shared_ptr<Epoll> GetInstance();

        Epoll(int maxevents);
        Epoll() : Epoll(kMaxEvents) {};
        ~Epoll();

        Epoll(const Epoll &) = delete;
        Epoll(Epoll &&) = delete;

        /** @brief add a fd to epoll
         * @param sock_fd fd
         * @param poller_ptr poll object
         */
        void Join(int sock_fd, std::shared_ptr<Pollable> poller_ptr);
        /** @brief remove a fd from epoll
         * @param sock_fd fd
         */
        void Leave(int sock_fd);

        /** @brief Indicate that data is ready to be sent out socket
         * @param sock_fd The socket id
         */
        void DataOutReady(int sock_fd);
        /** @brief Indicate no data is ready to be sent out socket
         * @param sock_fd The socket id
         */
        void DataOutClear(int sock_fd);

        /** @brief Indicate that reads from the socket will be serviced
         * @param sock_fd The socket id
         */
        void DataInResume(int sock_fd);
        /** @brief Indicate that the reads from the socket should be disabled
         * @param sock_fd The socket id
         */
        void DataInPause(int sock_fd);

        /** @brief start epoll loop
         */
        void Wait();
        /** @brief stop epoll loop
         */
        void Terminate();

    private:
        int epollfd_;
        int efd_;
        int maxevents_;
        std::atomic<bool> loop_flag_;

        /** @brief 根据 Pollable 类型组合构建 epoll_event 的 events 字段
         *  @param poller_ptr 对应的 Pollable 对象
         *  @param want_out   是否需要 EPOLLOUT（用于控制写就绪）
         *  @param want_in    是否需要 EPOLLIN（用于 DataInPause）
         *  @return 构建好的 epoll_event
         */
        struct epoll_event buildEpollEvents(const std::shared_ptr<Pollable> &poller_ptr,
                                            bool want_out = true,
                                            bool want_in = true);

        /** @brief 消费 eventfd，清空唤醒信号 */
        void onEventFd();

        // handle tcp accept event
        void onSocketAccept(int listen_sock);
        // handle socket close event
        void onSocketClose(int sock_fd);

        // handle tcp socket readable event(read())
        void onSocketRead(int sock_fd);
        // handle tcp socket writeable event(write())
        void onSocketWrite(int sock_fd);

        std::unordered_map<int, std::shared_ptr<Pollable>> fd_pollers_;
        std::recursive_mutex mutex_;
    };
}