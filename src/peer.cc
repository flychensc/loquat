#include <sstream>
#include <stdexcept>

#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "peer.h"

namespace loquat
{
    using namespace std;

    Peer::Peer(int domain) :
        domain_(domain)
    {
        sock_fd_ = ::socket(domain_, SOCK_DGRAM | SOCK_NONBLOCK, 0);
        if (sock_fd_ == -1)
        {
            stringstream errinfo;
            errinfo << "socket:" << strerror(errno);
            throw runtime_error(errinfo.str());
        }
    }

    Peer::~Peer()
    {
        ::close(sock_fd_);
    }
}