#include <cassert>
#include <sstream>
#include <stdexcept>

#include <errno.h>
#include <string.h>
#include <sys/socket.h>

#include "datagram.h"

namespace loquat
{
    using namespace std;

    void Datagram::Enqueue(struct sockaddr &toaddr, socklen_t addrlen, const vector<Byte> &data)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto &outbuf = io_buffer_.write_queue_;
        outbuf.push_back(make_tuple(toaddr, addrlen, data));
    }

    void Datagram::OnWrite(int sock_fd)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto &outbuf = io_buffer_.write_queue_;

        while (!outbuf.empty())
        {
            auto entry = outbuf.front();
            auto &dest_addr = get<0>(entry);
            auto &addrlen = get<1>(entry);
            auto &msg = get<2>(entry);

            auto written = ::sendto(sock_fd, msg.data(), msg.size(), 0, &dest_addr, addrlen);

            if (written < 0)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    written = 0;
                    return;
                }
                else
                {
                    stringstream errinfo;
                    errinfo << "sendto:" << strerror(errno);
                    throw runtime_error(errinfo.str());
                }
            }
            // [Q] this branch is unreachable, right?
            assert(written == msg.size());

            outbuf.pop_front();
        }
    }

    void Datagram::OnRead(int sock_fd)
    {
        struct sockaddr src_addr;
        socklen_t addrlen;

        auto &inbuf = io_buffer_.read_buffer_;

        auto bytes_in = ::recvfrom(sock_fd, inbuf.data(), inbuf.size(), 0, &src_addr, &addrlen);
        if (bytes_in < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                return;
            }

            stringstream errinfo;
            errinfo << "recvfrom:" << strerror(errno);
            throw runtime_error(errinfo.str());
        }

        io_buffer_.read_bytes_ = bytes_in;

        vector<Byte> recv_data;
        recv_data.assign(io_buffer_.read_buffer_.begin(), io_buffer_.read_buffer_.begin() + io_buffer_.read_bytes_);

        io_buffer_.read_bytes_ = 0;

        // invoke callback
        OnRecv(src_addr, addrlen, recv_data);
    }
}