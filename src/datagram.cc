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

    void Datagram::Enqueue(const SockAddr &toaddr, const vector<Byte> &data)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto &outbuf = io_buffer_.write_queue_;
        outbuf.push_back(make_tuple(toaddr, data));
    }

    void Datagram::OnWrite(int sock_fd)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto &outbuf = io_buffer_.write_queue_;

        if (!outbuf.empty())
        {
            auto &entry = outbuf.front();
            auto &dest_addr = get<0>(entry);
            auto &msg = get<1>(entry);

            auto written = ::sendto(sock_fd, msg.data(), msg.size(), 0, &dest_addr.addr.sa, dest_addr.addrlen);

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

    int Datagram::PktsEnqueued(void)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto &outbuf = io_buffer_.write_queue_;

        return outbuf.size();
    }

    void Datagram::OnRead(int sock_fd)
    {
        SockAddr src_addr;
        src_addr.addrlen = sizeof(SockAddr);

        auto &inbuf = io_buffer_.read_buffer_;

        auto bytes_in = ::recvfrom(sock_fd, inbuf.data(), inbuf.size(), 0, &src_addr.addr.sa, &src_addr.addrlen);
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
        OnRecv(src_addr, recv_data);
    }
}