#include <stdexcept>

#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "epoll.h"

namespace loquat
{
    using namespace std;

    void SetNonBlock(int sfd)
    {
        fcntl(sfd, F_SETFL, fcntl(sfd, F_GETFL) | O_NONBLOCK);
    }

    Epoll::Epoll(int maxevents) :
        maxevents_(maxevents)
    {
        epollfd_ = epoll_create1(0);
        if (epollfd_ == -1)
            throw runtime_error("epoll_create1");
    }

    Epoll::~Epoll()
    {
        close(epollfd_);
    }

    void Epoll::Join(int listen_sock, callback_accept_t accept_callback)
    {
        struct epoll_event ev;

        /*1.insert*/
        fd_callback_.insert({listen_sock, EventOPs(accept_callback, nullptr, nullptr, nullptr)});

        /*2.add to epoll*/
        ev.events = EPOLLIN;
        ev.data.fd = listen_sock;

        if (epoll_ctl(epollfd_, EPOLL_CTL_ADD, listen_sock, &ev) == -1)
            throw runtime_error("epoll_ctl: EPOLL_CTL_ADD(listen_sock)");
    }

    void Epoll::Join(int conn_sock, callback_recv_t recv_callback, callback_send_t send_callback, callback_close_t close_callback)
    {
        struct epoll_event ev;

        /*1.insert*/
        fd_callback_.insert({conn_sock, EventOPs(nullptr, recv_callback, send_callback, close_callback)});

        /*2.add to epoll*/
        ev.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLHUP;
        ev.data.fd = conn_sock;

        if (epoll_ctl(epollfd_, EPOLL_CTL_ADD, conn_sock, &ev) == -1)
            throw runtime_error("epoll_ctl: EPOLL_CTL_ADD(conn_sock)");
    }

    void Epoll::Join(int peer_sock, callback_recv_t recv_callback, callback_send_t send_callback)
    {
        struct epoll_event ev;

        /*1.insert*/
        fd_callback_.insert({peer_sock, EventOPs(nullptr, recv_callback, send_callback, nullptr)});

        /*2.add to epoll*/
        ev.events = EPOLLIN | EPOLLOUT;
        ev.data.fd = peer_sock;

        if (epoll_ctl(epollfd_, EPOLL_CTL_ADD, peer_sock, &ev) == -1)
            throw runtime_error("epoll_ctl: EPOLL_CTL_ADD(peer_sock)");
    }

    void Epoll::Leave(int sock_fd)
    {
        /*1.delete from epoll*/
        if (epoll_ctl(epollfd_, EPOLL_CTL_DEL, sock_fd, NULL) == -1)
            throw runtime_error("epoll_ctl: EPOLL_CTL_DEL(sock_fd)");

        /*2. erase*/
        fd_callback_.erase(sock_fd);
    }

    void Epoll::Wait()
    {
        int i, nfds;
        struct epoll_event events[maxevents_];

        for(;;)
        {
            nfds = epoll_wait(epollfd_, events, maxevents_, -1);

            for(i = 0; i < nfds; ++i)
            {
                if (isListenFd(events[i].data.fd))
                {
                    OnSocketAccept(events[i].data.fd);
                }

                if (events[i].events & EPOLLIN)
                {
                    OnSocketRead(events[i].data.fd);
                }

                if (events[i].events & EPOLLOUT)
                {
                    OnSocketWrite(events[i].data.fd);
                }

                if (events[i].events & (EPOLLRDHUP | EPOLLHUP))
                {
                    onSocketClose(events[i].data.fd);
                }
            }
        }
    }

    void Epoll::OnSocketAccept(int listen_sock)
    {
        int conn_fd;
        struct sockaddr_in addr = {0};
        socklen_t addrlen;
        struct epoll_event ev;

        /*1.accept new connection*/
        conn_fd = accept(listen_sock, (struct sockaddr *)&addr, &addrlen);
        if(conn_fd == -1)
            throw runtime_error("accept");

        /*2.set non-block*/
        SetNonBlock(conn_fd);

        /*3. lookup*/
        if (fd_callback_.count(listen_sock) == 1)
        {
            auto accept_callback = get<0>(fd_callback_.at(listen_sock));
            if (accept_callback != nullptr)
            {
                /*4. callback*/
                accept_callback(conn_fd);
            }
        }
    }

    void Epoll::onSocketClose(int sock_fd)
    {
        /*1.delete from epoll*/
        if (epoll_ctl(epollfd_, EPOLL_CTL_DEL, sock_fd, NULL) == -1)
            throw runtime_error("epoll_ctl: EPOLL_CTL_DEL(sock_fd)");

        /*1.close socket*/
        close(sock_fd);
        /*2. lookup*/
        if (fd_callback_.count(sock_fd) == 1)
        {
            auto close_callback = get<3>(fd_callback_.at(sock_fd));
            if (close_callback != nullptr)
            {
                /*3. callback*/
                close_callback(sock_fd);
            }
        }
        /*4. erase*/
        fd_callback_.erase(sock_fd);
    }

    void Epoll::OnSocketRead(int sock_fd)
    {
        /*1. lookup*/
        if (fd_callback_.count(sock_fd) == 1)
        {
            auto recv_callback = get<1>(fd_callback_.at(sock_fd));
            if (recv_callback != nullptr)
            {
                /*2. callback*/
                recv_callback(sock_fd);
            }
        }
    }

    void Epoll::OnSocketWrite(int sock_fd)
    {
        /*1. lookup*/
        if (fd_callback_.count(sock_fd) == 1)
        {
            auto send_callback = get<2>(fd_callback_.at(sock_fd));
            if (send_callback != nullptr)
            {
                /*2. callback*/
                send_callback(sock_fd);
            }
        }
    }

    bool Epoll::isListenFd(int fd)
    {
        /*1. lookup*/
        if (fd_callback_.count(fd) == 1)
        {
            auto accept_callback = get<0>(fd_callback_.at(fd));
            return accept_callback != nullptr;
        }
        return false;
    }
}