#include <iostream>
#include <string>
#include <vector>

#include <arpa/inet.h>
#include <netinet/in.h>

#include "epoll.h"
#include "peer.h"

using namespace loquat;

class PeerC : public Peer
{
public:
    void OnRecv(struct sockaddr &fromaddr, socklen_t addrlen, const std::vector<Byte> &data) override
    {
        char from_string[128];
        struct sockaddr_in *from = (struct sockaddr_in *)&fromaddr;

        ::inet_ntop(AF_INET, &from->sin_addr, from_string, 128);

        std::cout << "Receive from " << from_string << ":" << ::ntohs(from->sin_port) << " " << data.size() << " bytes: ";
        std::cout << data.data() << std::endl;

        Epoll::GetInstance().Terminate();
    }
};

int main(int argc,     // Number of strings in array argv
         char *argv[], // Array of command-line argument strings
         char *envp[]) // Array of environment variable strings
{
    auto p_peer_c = std::make_shared<PeerC>();

    Epoll::GetInstance().Join(p_peer_c->Sock(), p_peer_c);

    auto msg = std::string("Hello World");
    std::vector<Byte> data(msg.data(), msg.data() + msg.size());
    std::cout << "Send " << data.size() << " bytes: ";
    std::cout << data.data() << std::endl
              << std::endl;
    p_peer_c->Enqueue("127.0.0.1", 30018, data);

    p_peer_c->Bind("127.0.0.1", 30058);

    Epoll::GetInstance().Wait();

    Epoll::GetInstance().Leave(p_peer_c->Sock());

    return 0;
}