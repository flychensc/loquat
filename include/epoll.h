#pragma once

#include <functional>
#include <map>

namespace loquat
{
    using callback_accept_t = std::function<void(int conn_sock)>;
    using callback_recv_t = std::function<void(int sock)>;
    using callback_send_t = std::function<void(int sock)>;
    using callback_close_t = std::function<void(int sock)>;

    struct EventOPs
    {
        callback_accept_t accept_callback_;
        callback_recv_t recv_callback_;
        callback_send_t send_callback_;
        callback_close_t close_callback_;
    };

    class Epoll
    {
        friend void SetNonBlock(int sfd);

        public:
            static const int kMaxEvents = 20;

            Epoll(int maxevents);
            Epoll() : Epoll(kMaxEvents) {};
            ~Epoll();

            void Join(int listen_sock, callback_accept_t accept_callback, callback_close_t close_callback);
            void Join(int conn_sock, callback_recv_t recv_callback, callback_send_t send_callback, callback_close_t close_callback);
            void Leave(int sock_fd);

            void Wait();
        private:
            int epollfd_;
            int maxevents_;

            // handle tcp accept event
            void OnSocketAccept(int listen_sock);

            /*new connection established*/
            void onConnect(int conn_sock);
            /*connection closed*/
            void onDisconnect(int sock_fd);

            // handle tcp socket readable event(read())
            void OnSocketRead(int sock_fd);
            // handle tcp socket writeable event(write())
            void OnSocketWrite(int sock_fd);

            std::map<int, EventOPs> ctrl_fds_;

            bool isListenFd(int fd);
    };
}