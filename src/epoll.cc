#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <vector>

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
            std::stringstream errinfo;
            errinfo << "epoll_create1:" << strerror(errno);
            throw std::runtime_error(errinfo.str());
        }

        efd_ = eventfd(0, EFD_NONBLOCK);
        if (efd_ == -1)
        {
            std::stringstream errinfo;
            errinfo << "Failed to create eventfd:" << strerror(errno);
            throw std::runtime_error(errinfo.str());
        }

        struct epoll_event ev = {};
        ev.events = EPOLLIN;
        ev.data.fd = efd_;
        if (epoll_ctl(epollfd_, EPOLL_CTL_ADD, efd_, &ev) == -1)
        {
            std::stringstream errinfo;
            errinfo << "Failed to add eventfd to epoll:" << strerror(errno);
            throw std::runtime_error(errinfo.str());
        }

        spdlog::debug("Epoll:{}", epollfd_);
    }

    Epoll::~Epoll()
    {
        ::close(efd_);
        ::close(epollfd_);
        spdlog::debug("~Epoll:{}", epollfd_);
    }

    struct epoll_event Epoll::buildEpollEvents(const std::shared_ptr<Pollable> &poller_ptr,
                                               bool want_out,
                                               bool want_in)
    {
        struct epoll_event ev = {};

        auto acceptable_ptr = std::dynamic_pointer_cast<Acceptable>(poller_ptr);
        if (acceptable_ptr)
        {
            ev.events |= EPOLLIN;
        }
        auto closable_ptr = std::dynamic_pointer_cast<Closable>(poller_ptr);
        if (closable_ptr)
        {
            ev.events |= EPOLLRDHUP | EPOLLHUP;
        }
        auto readwritable_ptr = std::dynamic_pointer_cast<ReadWritable>(poller_ptr);
        if (readwritable_ptr)
        {
            if (want_in)
                ev.events |= EPOLLIN;
            if (want_out)
                ev.events |= EPOLLOUT;
        }

        return ev;
    }

    void Epoll::Join(int sock_fd, std::shared_ptr<Pollable> poller_ptr)
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);

        /*1. insert*/
        fd_pollers_.insert({sock_fd, poller_ptr});

        /*2. build events and add to epoll*/
        struct epoll_event ev = buildEpollEvents(poller_ptr);
        ev.data.fd = sock_fd;

        if (::epoll_ctl(epollfd_, EPOLL_CTL_ADD, sock_fd, &ev) == -1)
        {
            std::ostringstream errinfo;
            errinfo << "epoll_ctl: EPOLL_CTL_ADD(Join):" << strerror(errno);
            throw std::runtime_error(errinfo.str());
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
        if (::epoll_ctl(epollfd_, EPOLL_CTL_DEL, sock_fd, nullptr) == -1)
        {
            std::ostringstream errinfo;
            errinfo << "epoll_ctl: EPOLL_CTL_DEL(Leave):" << strerror(errno);
            throw std::runtime_error(errinfo.str());
        }

        /*2. erase*/
        fd_pollers_.erase(sock_fd);

        spdlog::debug("Epoll Leave:{}", sock_fd);
    }

    void Epoll::DataOutReady(int sock_fd)
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);

        auto poller_ptr = fd_pollers_.at(sock_fd);
        struct epoll_event ev = buildEpollEvents(poller_ptr, /*want_out=*/true, /*want_in=*/true);
        ev.data.fd = sock_fd;

        if (::epoll_ctl(epollfd_, EPOLL_CTL_MOD, sock_fd, &ev) == -1)
        {
            std::ostringstream errinfo;
            errinfo << "epoll_ctl: EPOLL_CTL_MOD(DataOutReady):" << strerror(errno);
            throw std::runtime_error(errinfo.str());
        }

        spdlog::debug("Epoll DataOutReady:{} with events:0x{:X}", sock_fd, static_cast<unsigned int>(ev.events));
    }

    void Epoll::DataOutClear(int sock_fd)
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);

        auto poller_ptr = fd_pollers_.at(sock_fd);
        struct epoll_event ev = buildEpollEvents(poller_ptr, /*want_out=*/false, /*want_in=*/true);
        ev.data.fd = sock_fd;

        if (::epoll_ctl(epollfd_, EPOLL_CTL_MOD, sock_fd, &ev) == -1)
        {
            std::ostringstream errinfo;
            errinfo << "epoll_ctl: EPOLL_CTL_MOD(DataOutClear):" << strerror(errno);
            throw std::runtime_error(errinfo.str());
        }

        spdlog::debug("Epoll DataOutClear:{} with events:0x{:X}", sock_fd, static_cast<unsigned int>(ev.events));
    }

    void Epoll::DataInResume(int sock_fd)
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);

        auto poller_ptr = fd_pollers_.at(sock_fd);
        struct epoll_event ev = buildEpollEvents(poller_ptr, /*want_out=*/true, /*want_in=*/true);
        ev.data.fd = sock_fd;

        if (::epoll_ctl(epollfd_, EPOLL_CTL_MOD, sock_fd, &ev) == -1)
        {
            std::ostringstream errinfo;
            errinfo << "epoll_ctl: EPOLL_CTL_MOD(DataInResume):" << strerror(errno);
            throw std::runtime_error(errinfo.str());
        }

        spdlog::debug("Epoll DataInResume:{} with events:0x{:X}", sock_fd, static_cast<unsigned int>(ev.events));
    }

    void Epoll::DataInPause(int sock_fd)
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);

        auto poller_ptr = fd_pollers_.at(sock_fd);
        struct epoll_event ev = buildEpollEvents(poller_ptr, /*want_out=*/true, /*want_in=*/false);
        ev.data.fd = sock_fd;

        if (::epoll_ctl(epollfd_, EPOLL_CTL_MOD, sock_fd, &ev) == -1)
        {
            std::ostringstream errinfo;
            errinfo << "epoll_ctl: EPOLL_CTL_MOD(DataInPause):" << strerror(errno);
            throw std::runtime_error(errinfo.str());
        }

        spdlog::debug("Epoll DataInPause:{} with events:0x{:X}", sock_fd, static_cast<unsigned int>(ev.events));
    }

    void Epoll::onEventFd()
    {
        uint64_t val = 0;
        [[maybe_unused]] ssize_t n = ::read(efd_, &val, sizeof(val)); // consume the wakeup signal
    }

    void Epoll::Wait()
    {
        std::vector<struct epoll_event> events(maxevents_);

        assert(!fd_pollers_.empty());
        loop_flag_ = true;
        while (loop_flag_.load(std::memory_order_acquire))
        {
            int nfds = ::epoll_wait(epollfd_, events.data(), maxevents_, -1);

            if (nfds < 0)
            {
                if (errno == EINTR)
                    continue;
                std::ostringstream errinfo;
                errinfo << "epoll_wait:" << strerror(errno);
                throw std::runtime_error(errinfo.str());
            }

            for (int i = 0; i < nfds; ++i)
            {
                int fd = events[i].data.fd;

                // Handle eventfd wakeup (Terminate signal)
                if (fd == efd_)
                {
                    onEventFd();
                    continue;
                }

                std::lock_guard<std::recursive_mutex> lock(mutex_);

                auto it = fd_pollers_.find(fd);
                if (it == fd_pollers_.end())
                    continue;

                auto poller_ptr = it->second;
                auto acceptable_ptr = std::dynamic_pointer_cast<Acceptable>(poller_ptr);
                if (acceptable_ptr)
                {
                    onSocketAccept(fd);
                }
                else
                {
                    if (events[i].events & EPOLLHUP)
                    {
                        // confirm socket state
                        ssize_t result = ::recv(fd, nullptr, 0, MSG_DONTWAIT);
                        if (result < 0)
                        {
                            if ((errno != ENOTCONN) && (errno != EAGAIN) && (errno != EWOULDBLOCK))
                            {
                                onSocketClose(fd);
                                continue;
                            }
                        }
                    }
                    if (events[i].events & EPOLLRDHUP)
                    {
                        onSocketClose(fd);
                        continue;
                    }

                    if (events[i].events & EPOLLIN)
                    {
                        onSocketRead(fd);
                    }

                    if (events[i].events & EPOLLOUT)
                    {
                        onSocketWrite(fd);
                    }
                }
            }
        }
    }

    void Epoll::Terminate()
    {
        loop_flag_.store(false, std::memory_order_release);
        uint64_t value = 1;
        [[maybe_unused]] ssize_t n = ::write(efd_, &value, sizeof(value)); // wake up epoll_wait
    }

    void Epoll::onSocketAccept(int listen_sock)
    {
        /*1. lookup*/
        auto poller_ptr = fd_pollers_.at(listen_sock);
        auto acceptable_ptr = std::dynamic_pointer_cast<Acceptable>(poller_ptr);
        if (acceptable_ptr)
        {
            /*2. callback*/
            acceptable_ptr->OnAccept(listen_sock);
        }
    }

    void Epoll::onSocketClose(int sock_fd)
    {
        /*1.delete from epoll*/
        if (::epoll_ctl(epollfd_, EPOLL_CTL_DEL, sock_fd, nullptr) == -1)
        {
            std::ostringstream errinfo;
            errinfo << "epoll_ctl: EPOLL_CTL_DEL(onSocketClose):" << strerror(errno);
            throw std::runtime_error(errinfo.str());
        }

        /*2. lookup*/
        auto poller_ptr = fd_pollers_.at(sock_fd);
        /*3. erase*/
        fd_pollers_.erase(sock_fd);

        spdlog::debug("Epoll onSocketClose:{}", sock_fd);

        auto closable_ptr = std::dynamic_pointer_cast<Closable>(poller_ptr);
        if (closable_ptr)
        {
            /*4. callback*/
            closable_ptr->OnClose(sock_fd);
        }

        // stop wait if no fd set
        if (fd_pollers_.empty())
        {
            loop_flag_.store(false, std::memory_order_release);
        }
    }

    void Epoll::onSocketRead(int sock_fd)
    {
        /*1. lookup*/
        auto poller_ptr = fd_pollers_.at(sock_fd);
        auto readwritable_ptr = std::dynamic_pointer_cast<ReadWritable>(poller_ptr);
        if (readwritable_ptr)
        {
            /*2. callback*/
            readwritable_ptr->OnRead(sock_fd);
        }
    }

    void Epoll::onSocketWrite(int sock_fd)
    {
        /*1. lookup*/
        auto poller_ptr = fd_pollers_.at(sock_fd);
        auto readwritable_ptr = std::dynamic_pointer_cast<ReadWritable>(poller_ptr);
        if (readwritable_ptr)
        {
            /*2. callback*/
            readwritable_ptr->OnWrite(sock_fd);
        }
    }
}