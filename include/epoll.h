#pragma once

#include <atomic>
#include <map>
#include <memory>

#include "pollable.h"

namespace loquat
{
    class Epoll
    {
    public:
        static const int kMaxEvents = 20;

        static Epoll &GetInstance();

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

        /** @brief start epoll loop
         */
        void Wait();
        /** @brief stop epoll loop
         */
        void Terminate();

    private:
        int epollfd_;
        int maxevents_;
        std::atomic<bool> loop_flag_;

        // handle tcp accept event
        void onSocketAccept(int listen_sock);
        // handle socket close event
        void onSocketClose(int sock_fd);

        // handle tcp socket readable event(read())
        void onSocketRead(int sock_fd);
        // handle tcp socket writeable event(write())
        void onSocketWrite(int sock_fd);

        std::map<int, std::shared_ptr<Pollable>> fd_pollers_;
    };
}