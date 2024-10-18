#include "listener.h"

#include "gtest/gtest.h"
namespace
{
    class TestListener : public loquat::Listener
    {
    public:
        TestListener() : loquat::Listener(10) {};

        void OnAccept(int listen_sock) {}
    };

    TEST(Listener, destructor)
    {
        int socket;

        {
            TestListener listener;
            socket = listener.Sock();
        }

        int error;
        socklen_t len = sizeof(error);
        int rv = getsockopt(socket, SOL_SOCKET, SO_ERROR, &error, &len);
        EXPECT_EQ(rv, -1);
    }
}
