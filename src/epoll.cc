#include <sstream>
#include <stdexcept>

#include <errno.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "epoll.h"

namespace loquat
{
    using namespace std;

    Epoll& Epoll::GetInstance()
    {
        static Epoll single;
        return single;
    }

    Epoll::Epoll(int maxevents) :
        maxevents_(maxevents)
    {
        epollfd_ = ::epoll_create1(0);
        if (epollfd_ == -1)
        {
            stringstream errinfo;
            errinfo << "epoll_create1:" << strerror(errno);
            throw runtime_error(errinfo.str());
        }
    }

    Epoll::~Epoll()
    {
        ::close(epollfd_);
    }

    void Epoll::Join(int sock_fd, shared_ptr<Pollable> poller_ptr)
    {
        struct epoll_event ev = {0};

        /*1. insert*/
        fd_pollers_.insert({sock_fd, poller_ptr});

        /*2.add to epoll*/
        auto acceptable_ptr = dynamic_pointer_cast<Acceptable>(poller_ptr);
        if (acceptable_ptr)
        {
            ev.events |= EPOLLIN;
        }
        auto closalbe_ptr = dynamic_pointer_cast<Closable>(poller_ptr);
        if (closalbe_ptr)
        {
            ev.events |= EPOLLRDHUP | EPOLLHUP;
        }
        auto readwritalbe_ptr = dynamic_pointer_cast<ReadWritable>(poller_ptr);
        if (readwritalbe_ptr)
        {
            ev.events |= EPOLLIN | EPOLLOUT;
        }
        ev.data.fd = sock_fd;

        if (::epoll_ctl(epollfd_, EPOLL_CTL_ADD, sock_fd, &ev) == -1)
        {
            stringstream errinfo;
            errinfo << "epoll_ctl: EPOLL_CTL_ADD(Join):" << strerror(errno);
            throw runtime_error(errinfo.str());
        }
    }

    void Epoll::Leave(int sock_fd)
    {
        /*1.delete from epoll*/
        if (::epoll_ctl(epollfd_, EPOLL_CTL_DEL, sock_fd, NULL) == -1)
        {
            stringstream errinfo;
            errinfo << "epoll_ctl: EPOLL_CTL_DEL(Leave):" << strerror(errno);
            throw runtime_error(errinfo.str());
        }

        /*2. erase*/
        fd_pollers_.erase(sock_fd);
    }

    void Epoll::Wait()
    {
        int i, nfds;
        struct epoll_event events[maxevents_];

        loop_flag_ = true;
        while(loop_flag_)
        {
            nfds = ::epoll_wait(epollfd_, events, maxevents_, -1);

            for(i = 0; i < nfds; ++i)
            {
                auto poller_ptr = fd_pollers_.at(events[i].data.fd);
                auto acceptable_ptr = dynamic_pointer_cast<Acceptable>(poller_ptr);
                if (acceptable_ptr)
                {
                    onSocketAccept(events[i].data.fd);
                }
                else
                {
                    if (events[i].events & (EPOLLRDHUP | EPOLLHUP))
                    {
                        onSocketClose(events[i].data.fd);
                        continue;
                    }

                    if (events[i].events & EPOLLIN)
                    {
                        onSocketRead(events[i].data.fd);
                    }

                    if (events[i].events & EPOLLOUT)
                    {
                        onSocketWrite(events[i].data.fd);
                    }
                }
            }
        }
    }

    void Epoll::Terminate()
    {
        loop_flag_ = false;
    }

    void Epoll::onSocketAccept(int listen_sock)
    {
        /*1. lookup*/
        auto poller_ptr = fd_pollers_.at(listen_sock);
        auto acceptable_ptr = dynamic_pointer_cast<Acceptable>(poller_ptr);
        if (acceptable_ptr)
        {
            /*2. callback*/
            acceptable_ptr->OnAccept(listen_sock);
        }
    }

    void Epoll::onSocketClose(int sock_fd)
    {
        /*1.delete from epoll*/
        if (::epoll_ctl(epollfd_, EPOLL_CTL_DEL, sock_fd, NULL) == -1)
        {
            stringstream errinfo;
            errinfo << "epoll_ctl: EPOLL_CTL_DEL(onSocketClose):" << strerror(errno);
            throw runtime_error(errinfo.str());
        }

        /*2. lookup*/
        auto poller_ptr = fd_pollers_.at(sock_fd);
        /*3. erase*/
        fd_pollers_.erase(sock_fd);
        auto closalbe_ptr = dynamic_pointer_cast<Closable>(poller_ptr);
        if (closalbe_ptr)
        {
            /*4. callback*/
            closalbe_ptr->OnClose(sock_fd);
        }
    }

    void Epoll::onSocketRead(int sock_fd)
    {
        /*1. lookup*/
        auto poller_ptr = fd_pollers_.at(sock_fd);
        auto readwritalbe_ptr = dynamic_pointer_cast<ReadWritable>(poller_ptr);
        if (readwritalbe_ptr)
        {
            /*2. callback*/
            readwritalbe_ptr->OnRead(sock_fd);
        }
    }

    void Epoll::onSocketWrite(int sock_fd)
    {
        /*1. lookup*/
        auto poller_ptr = fd_pollers_.at(sock_fd);
        auto readwritalbe_ptr = dynamic_pointer_cast<ReadWritable>(poller_ptr);
        if (readwritalbe_ptr)
        {
            /*2. callback*/
            readwritalbe_ptr->OnWrite(sock_fd);
        }
    }
}