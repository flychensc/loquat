#include <sstream>
#include <stdexcept>

#include <errno.h>
#include <string.h>
#include <sys/socket.h>

#include "datagram.h"

namespace loquat
{
    using namespace std;

    void Datagram::Enqueue(std::string& to, std::vector<Byte>& data)
    {
        auto& outbuf = io_buffer_.write_queue_;
        outbuf.push_back(make_pair(to, data));
    }

    void Datagram::OnWrite(int sock_fd)
    {
        auto& outbuf = io_buffer_.write_queue_;

        while(!outbuf.empty())
        {
            auto& entry = outbuf.front();
            auto& to = entry.first;
            auto& msg = entry.second;

            struct sockaddr dest_addr;
            socklen_t addrlen;

            // todo:

            auto written = ::sendto(sock_fd, msg.data(), msg.size(), 0, &dest_addr, addrlen);

            if (written < 0)
            {
                if (errno == EAGAIN)
                {
                    written = 0;
                    return;
                }
                else
                {
                    stringstream errinfo;
                    errinfo << "send:" << strerror(errno);
                    throw runtime_error(errinfo.str());
                }
            }
            else if (written < msg.size())
            {
                // [Q] this branch is unreachable, right?
                return;
            }

            outbuf.pop_front();
        }
    }

    void Datagram::OnRead(int sock_fd)
    {
        auto& inbuf = io_buffer_.read_buffer_;

        auto buf = inbuf.data();
        auto len = inbuf.size();

        struct sockaddr src_addr;
        socklen_t addrlen;

        auto bytes_in = ::recvfrom(sock_fd, buf, len, 0, &src_addr, &addrlen);
        if (bytes_in <= 0)
        {
            if (bytes_in == 0)
            {
                /* Socket is closed */
                throw runtime_error("Stream is closed");
            }

            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                return;
            }

            stringstream errinfo;
            errinfo << "recv:" << strerror(errno);
            throw runtime_error(errinfo.str());
        }

        io_buffer_.read_bytes_ = bytes_in;

        vector<Byte> recv_data;
        recv_data.assign(io_buffer_.read_buffer_.begin(), io_buffer_.read_buffer_.begin()+io_buffer_.read_bytes_);

        io_buffer_.read_bytes_ = 0;

        string from;

        // todo:

        // invoke callback
        OnRecv(from, recv_data);
    }
}