#pragma once

#include <functional>
#include <map>

namespace loquat
{
    void SetNonBlock(int sfd);

    class Epoll
    {

        using callback_accept_t = std::function<void(int conn_sock)>;
        using callback_recv_t = std::function<void(int sock)>;
        using callback_send_t = std::function<void(int sock)>;
        using callback_close_t = std::function<void(int sock)>;

        using EventOPs = std::tuple<callback_accept_t, callback_recv_t, callback_send_t, callback_close_t>;

        public:
            static const int kMaxEvents = 20;

            Epoll(int maxevents);
            Epoll() : Epoll(kMaxEvents) {};
            ~Epoll();

            Epoll(const Epoll&) = delete;
            Epoll(Epoll&&) = delete;

            // for Listener
            void Join(int listen_sock, callback_accept_t accept_callback);
            // for Connector
            void Join(int conn_sock, callback_recv_t recv_callback, callback_send_t send_callback, callback_close_t close_callback);
            // for Peer
            void Join(int peer_sock, callback_recv_t recv_callback, callback_send_t send_callback);
            void Leave(int sock_fd);

            void Wait();
        private:
            int epollfd_;
            int maxevents_;

            // handle tcp accept event
            void OnSocketAccept(int listen_sock);
            // handle socket close event
            void onSocketClose(int sock_fd);

            // handle tcp socket readable event(read())
            void OnSocketRead(int sock_fd);
            // handle tcp socket writeable event(write())
            void OnSocketWrite(int sock_fd);

            std::map<int, EventOPs> fd_callback_;

            bool isListenFd(int fd);
    };
}