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

    void Epoll::Join(int listen_sock, callback_accept_t accept_callback, callback_close_t close_callback)
    {
        struct epoll_event ev;

        /*1.insert*/

        /*2.add to epoll*/
        ev.events = EPOLLIN;
        ev.data.fd = listen_sock;

        if (epoll_ctl(epollfd_, EPOLL_CTL_ADD, listen_sock, &ev) == -1)
            throw runtime_error("epoll_ctl: EPOLL_CTL_ADD(listen_sock)");
    }

    void Epoll::Leave(int sock_fd)
    {
        /*1.delete from epoll*/
        if (epoll_ctl(epollfd_, EPOLL_CTL_DEL, sock_fd, NULL) == -1)
            throw runtime_error("epoll_ctl: EPOLL_CTL_DEL(sock_fd)");

        /*2. erase*/
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
                    onDisconnect(events[i].data.fd);
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
        /*4. callback*/
    }

    void Epoll::onConnect(int conn_sock)
    {
        struct epoll_event ev;

        /*1.insert*/

        /*2.add to epoll*/
        ev.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLHUP;
        ev.data.fd = conn_sock;

        if (epoll_ctl(epollfd_, EPOLL_CTL_ADD, conn_sock, &ev) == -1)
            throw runtime_error("epoll_ctl: EPOLL_CTL_ADD(conn_sock)");
    }

    void Epoll::onDisconnect(int sock_fd)
    {
        /*1.delete from epoll*/
        if (epoll_ctl(epollfd_, EPOLL_CTL_DEL, sock_fd, NULL) == -1)
            throw runtime_error("epoll_ctl: EPOLL_CTL_DEL(sock_fd)");

        /*1.close socket*/
        close(sock_fd);
        /*2. lookup*/
        /*3. callback*/
        /*4. erase*/
    }

    void Epoll::OnSocketRead(int sock_fd)
    {
        /*1. lookup*/
        /*2. callback*/
    }

    void Epoll::OnSocketWrite(int sock_fd)
    {
        /*1. lookup*/
        /*2. callback*/
    }

    bool Epoll::isListenFd(int fd)
    {
        /*1. lookup*/
        // todo:
        return true;
    }
}