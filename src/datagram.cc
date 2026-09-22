#include <cassert>
#include <sstream>
#include <stdexcept>

#include <errno.h>
#include <string.h>
#include <sys/socket.h>

#include "datagram.h"

namespace loquat
{

    void Datagram::Enqueue(const SockAddr &toaddr, std::vector<Byte> data)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto &outbuf = io_buffer_.write_queue_;
        outbuf.push_back(std::make_tuple(toaddr, std::move(data)));
    }

    void Datagram::OnWrite(int sock_fd)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto &outbuf = io_buffer_.write_queue_;

        if (!outbuf.empty())
        {
            auto &entry = outbuf.front();
            auto &dest_addr = std::get<0>(entry);
            auto &msg = std::get<1>(entry);

            auto written = ::sendto(sock_fd, msg.data(), msg.size(), 0, &dest_addr.addr.sa, dest_addr.addrlen);

            if (written < 0)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    return;
                }
                else
                {
                    std::ostringstream errinfo;
                    errinfo << "sendto:" << strerror(errno);
                    throw std::runtime_error(errinfo.str());
                }
            }

            outbuf.pop_front();
        }
    }

    int Datagram::PktsEnqueued(void)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto &outbuf = io_buffer_.write_queue_;

        return static_cast<int>(outbuf.size());
    }

    void Datagram::OnRead(int sock_fd)
    {
        SockAddr src_addr;
        src_addr.addrlen = sizeof(SockAddr);

        std::vector<Byte> recv_data;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto &inbuf = io_buffer_.read_buffer_;

            auto bytes_in = ::recvfrom(sock_fd, inbuf.data(), inbuf.size(), 0, &src_addr.addr.sa, &src_addr.addrlen);
            if (bytes_in < 0)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    return;
                }

                std::ostringstream errinfo;
                errinfo << "recvfrom:" << strerror(errno);
                throw std::runtime_error(errinfo.str());
            }

            io_buffer_.read_bytes_ = bytes_in;
            recv_data.assign(io_buffer_.read_buffer_.begin(),
                             io_buffer_.read_buffer_.begin() + io_buffer_.read_bytes_);
            io_buffer_.read_bytes_ = 0;
        }

        // invoke callback outside the lock
        OnRecv(src_addr, std::move(recv_data));
    }
}