#include <iostream>
#include <string>
#include <vector>

#include <arpa/inet.h>
#include <netinet/in.h>

#include "epoll.h"
#include "peer.h"

using namespace loquat;

class PeerS : public Peer
{
public:
    void OnRecv(const SockAddr &fromaddr, const std::vector<Byte> &data) override
    {
        char from_string[128];
        struct sockaddr_in *from = (struct sockaddr_in *)&fromaddr;

        ::inet_ntop(AF_INET, &from->sin_addr, from_string, 128);

        std::cout << "Receive from " << from_string << ":" << ::ntohs(from->sin_port) << " " << data.size() << " bytes: ";
        std::cout << data.data() << std::endl;

        // Echo back
        Enqueue(fromaddr, data);
    }
};

int main(int argc,     // Number of strings in array argv
         char *argv[], // Array of command-line argument strings
         char *envp[]) // Array of environment variable strings
{
    auto p_peer_s = std::make_shared<PeerS>();

    Epoll::GetInstance()->Join(p_peer_s->Sock(), p_peer_s);

    p_peer_s->Bind("127.0.0.1", 30018);

    Epoll::GetInstance()->Wait();

    return 0;
}