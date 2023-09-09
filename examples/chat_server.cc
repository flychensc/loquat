#include <functional>
#include <iostream>

#include "epoll.h"
#include "listener.h"

using namespace loquat;

class ChatServer;

class ChatSession : public Connection
{
    public:
        ChatSession(int listen_fd, ChatServer *p_server) : Connection(listen_fd), server_(p_server) {};

        void OnClose(int sock_fd) override;
        void OnRecv(std::vector<Byte>& data) override;
    private:
        ChatServer *server_;
};

class ChatServer : public Listener
{
    public:
        static const int kMaxConnections = 50;
        ChatServer() : Listener(kMaxConnections) {};

        void OnAccept(int listen_sock) override;
        void Remove(int sess_fd) { sessions_.erase(sess_fd); };

        void Broadcast(std::vector<Byte>& data);
    private:
        std::map<int, std::shared_ptr<ChatSession>> sessions_;
};

void ChatSession::OnClose(int sock_fd)
{
    std::cout << "[Session] " << sock_fd << " closed" << std::endl;
    server_->Remove(sock_fd);
}

void ChatSession::OnRecv(std::vector<Byte>& data)
{
    server_->Broadcast(data);
}

void ChatServer::OnAccept(int listen_sock)
{
    auto connection_ptr = std::make_shared<ChatSession>(listen_sock, this);
    std::cout << "[Session] " << connection_ptr->Sock() << " established" << std::endl;

    sessions_.insert({connection_ptr->Sock(), connection_ptr});

    Epoll::GetInstance().Join(connection_ptr->Sock(), connection_ptr);
}

void ChatServer::Broadcast(std::vector<Byte>& data)
{
    std::cout << " " << data.data() << std::endl;
    for (auto& session : sessions_)
    {
        session.second->Enqueue(data);
    }
}

int main( int argc,      // Number of strings in array argv
        char *argv[],   // Array of command-line argument strings
        char *envp[] )  // Array of environment variable strings
{
    auto p_serverd = std::make_shared<ChatServer>();

    Epoll::GetInstance().Join(p_serverd->Sock(), p_serverd);

    p_serverd->Listen("0.0.0.0", 300158);

    Epoll::GetInstance().Wait();

    return 0;
}