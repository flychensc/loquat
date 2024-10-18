#include "peer.h"

#include "gtest/gtest.h"
namespace
{
    class TestPeer : public loquat::Peer
    {
    public:
        void OnRecv(struct sockaddr &fromaddr, socklen_t addrlen, std::vector<loquat::Byte> &data) override {}
    };

    TEST(Peer, destructor)
    {
        int socket;

        {
            TestPeer peer;
            socket = peer.Sock();
        }

        int error;
        socklen_t len = sizeof(error);
        int rv = getsockopt(socket, SOL_SOCKET, SO_ERROR, &error, &len);
        EXPECT_EQ(rv, -1);
    }
}
