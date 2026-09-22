#pragma once

namespace loquat
{
    class Epoll; // 前向声明

    class Pollable
    {
    public:
        virtual ~Pollable() = default;

        /** @brief 设置所属的 Epoll 实例（由 Epoll::Join 调用） */
        void setEpoll(Epoll *epoll) { epoll_ = epoll; }
        /** @brief 获取所属的 Epoll 实例 */
        Epoll *epoll() const { return epoll_; }

    protected:
        Epoll *epoll_ = nullptr;
    };

    class Acceptable : virtual public Pollable
    {
    protected:
        /** @brief register a callback to handle new connection
         *  @param listen_sock listening sock id
         */
        virtual void OnAccept(int listen_sock) = 0;

        /** @brief 监听 socket 发生错误
         *  @param listen_sock listening sock id
         */
        virtual void OnAcceptError(int listen_sock) { (void)listen_sock; }

        friend class Epoll;
    };

    class ReadWritable : virtual public Pollable
    {
    protected:
        /** @brief register a callback to handle message in
         *  @param sock_fd sock id
         */
        virtual void OnRead(int sock_fd) = 0;
        /** @brief register a callback to handle message out
         *  @param sock_fd sock id
         */
        virtual void OnWrite(int sock_fd) = 0;

        /** @brief 对端主动关闭连接时的通知（默认空实现）
         *  @param sock_fd sock id
         */
        virtual void OnDisconnect(int sock_fd) { (void)sock_fd; }

        friend class Epoll;
    };

    class Closable : virtual public Pollable
    {
    protected:
        /** @brief register a callback to handle connection disconnected
         *  @param sock_fd sock id
         */
        virtual void OnClose(int sock_fd) = 0;
        friend class Epoll;
    };
}