#include <sstream>
#include <stdexcept>

#include <errno.h>
#include <netinet/in.h>
#include <string.h>
#include <spdlog/spdlog.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <unistd.h>

#include "epoll.h"

namespace loquat
{
    using namespace std;

    std::shared_ptr<Epoll> Epoll::GetInstance()
    {
        static std::shared_ptr<Epoll> single = std::make_shared<Epoll>();
        return single;
    }

    Epoll::Epoll(int maxevents) : maxevents_(maxevents), loop_flag_(false)
    {
        epollfd_ = ::epoll_create1(0);
        if (epollfd_ == -1)
        {
            stringstream errinfo;
            errinfo << "epoll_create1:" << strerror(errno);
            throw runtime_error(errinfo.str());
        }

        efd_ = eventfd(0, EFD_NONBLOCK);
        if (efd_ == -1)
        {
            stringstream errinfo;
            errinfo << "Failed to create eventfd:" << strerror(errno);
            throw runtime_error(errinfo.str());
        }

        struct epoll_event ev = {0};
        ev.events = EPOLLIN;
        ev.data.fd = efd_;
        if (epoll_ctl(epollfd_, EPOLL_CTL_ADD, efd_, &ev) == -1)
        {
            stringstream errinfo;
            errinfo << "Failed to add eventfd to epoll:" << strerror(errno);
            throw runtime_error(errinfo.str());
        }

        spdlog::debug("Epoll:{}", epollfd_);
    }

    Epoll::~Epoll()
    {
        ::close(efd_);
        ::close(epollfd_);
        spdlog::debug("~Epoll:{}", epollfd_);
    }

    void Epoll::Join(int sock_fd, shared_ptr<Pollable> poller_ptr)
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);

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

        spdlog::debug("Epoll Join:{} with events:0x{:X}", sock_fd, static_cast<unsigned int>(ev.events));
    }

    void Epoll::Leave(int sock_fd)
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);

        if (fd_pollers_.find(sock_fd) == fd_pollers_.end())
        {
            return;
        }

        /*1.delete from epoll*/
        if (::epoll_ctl(epollfd_, EPOLL_CTL_DEL, sock_fd, NULL) == -1)
        {
            stringstream errinfo;
            errinfo << "epoll_ctl: EPOLL_CTL_DEL(Leave):" << strerror(errno);
            throw runtime_error(errinfo.str());
        }

        /*2. erase*/
        fd_pollers_.erase(sock_fd);

        spdlog::debug("Epoll Leave:{}", sock_fd);
    }

    void Epoll::DataOutReady(int sock_fd)
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);

        struct epoll_event ev = {0};

        /*1. locate*/
        auto poller_ptr = fd_pollers_.at(sock_fd);

        /*2. modify to epoll*/
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

        if (::epoll_ctl(epollfd_, EPOLL_CTL_MOD, sock_fd, &ev) == -1)
        {
            stringstream errinfo;
            errinfo << "epoll_ctl: EPOLL_CTL_MOD(DataOutReady):" << strerror(errno);
            throw runtime_error(errinfo.str());
        }

        spdlog::debug("Epoll DataOutReady:{} with events:0x{:X}", sock_fd, static_cast<unsigned int>(ev.events));
    }

    void Epoll::DataOutClear(int sock_fd)
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);

        struct epoll_event ev = {0};

        /*1. locate*/
        auto poller_ptr = fd_pollers_.at(sock_fd);

        /*2. modify to epoll*/
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
            ev.events |= EPOLLIN;
        }
        ev.data.fd = sock_fd;

        if (::epoll_ctl(epollfd_, EPOLL_CTL_MOD, sock_fd, &ev) == -1)
        {
            stringstream errinfo;
            errinfo << "epoll_ctl: EPOLL_CTL_MOD(DataOutClear):" << strerror(errno);
            throw runtime_error(errinfo.str());
        }

        spdlog::debug("Epoll DataOutClear:{} with events:0x{:X}", sock_fd, static_cast<unsigned int>(ev.events));
    }

    void Epoll::DataInResume(int sock_fd)
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);

        struct epoll_event ev = {0};

        /*1. locate*/
        auto poller_ptr = fd_pollers_.at(sock_fd);

        /*2. modify to epoll*/
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

        if (::epoll_ctl(epollfd_, EPOLL_CTL_MOD, sock_fd, &ev) == -1)
        {
            stringstream errinfo;
            errinfo << "epoll_ctl: EPOLL_CTL_MOD(DataInResume):" << strerror(errno);
            throw runtime_error(errinfo.str());
        }

        spdlog::debug("Epoll DataInResume:{} with events:0x{:X}", sock_fd, static_cast<unsigned int>(ev.events));
    }

    void Epoll::DataInPause(int sock_fd)
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);

        struct epoll_event ev = {0};

        /*1. locate*/
        auto poller_ptr = fd_pollers_.at(sock_fd);

        /*2. modify to epoll*/
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
            ev.events |= EPOLLOUT;
        }
        ev.data.fd = sock_fd;

        if (::epoll_ctl(epollfd_, EPOLL_CTL_MOD, sock_fd, &ev) == -1)
        {
            stringstream errinfo;
            errinfo << "epoll_ctl: EPOLL_CTL_MOD(DataInPause):" << strerror(errno);
            throw runtime_error(errinfo.str());
        }

        spdlog::debug("Epoll DataInPause:{} with events:0x{:X}", sock_fd, static_cast<unsigned int>(ev.events));
    }

    void Epoll::Wait()
    {
        int i, nfds;
        struct epoll_event events[maxevents_];

        assert(!fd_pollers_.empty());
        loop_flag_ = true;
        while (loop_flag_.load(std::memory_order_acquire))
        {
            nfds = ::epoll_wait(epollfd_, events, maxevents_, -1);

            for (i = 0; i < nfds; ++i)
            {
                std::lock_guard<std::recursive_mutex> lock(mutex_);

                if (fd_pollers_.find(events[i].data.fd) == fd_pollers_.end())
                {
                    continue;
                }

                auto poller_ptr = fd_pollers_.at(events[i].data.fd);
                auto acceptable_ptr = dynamic_pointer_cast<Acceptable>(poller_ptr);
                if (acceptable_ptr)
                {
                    onSocketAccept(events[i].data.fd);
                }
                else
                {
                    if (events[i].events & EPOLLHUP)
                    {
                        // confirm socket state
                        ssize_t result = recv(events[i].data.fd, nullptr, 0, MSG_DONTWAIT);
                        if (result < 0)
                        {
                            if ((errno != ENOTCONN) && (errno != EAGAIN) && (errno != EWOULDBLOCK))
                            {
                                onSocketClose(events[i].data.fd);
                                // ignore EPOLLOUT
                                continue;
                            }
                        }
                    }
                    if (events[i].events & EPOLLRDHUP)
                    {
                        onSocketClose(events[i].data.fd);
                        // ignore EPOLLOUT
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
        uint64_t value = 1;
        write(efd_, &value, sizeof(value));

        loop_flag_.store(false, std::memory_order_acquire);
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

        spdlog::debug("Epoll onSocketClose:{}", sock_fd);

        auto closalbe_ptr = dynamic_pointer_cast<Closable>(poller_ptr);
        if (closalbe_ptr)
        {
            /*4. callback*/
            closalbe_ptr->OnClose(sock_fd);
        }

        // stop wait if no fd set
        if (fd_pollers_.empty())
        {
            loop_flag_.store(false);
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