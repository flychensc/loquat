# loquat
With the polymorphic and inheritance features of C++, use Epoll to implement TCP server/client, and UDP.

## Usage

`Epoll` is the core of system. All fd(s) `Epoll.Join` to poll read/write events. The system started working with `Epoll.Wait`.

`Listener` is used to implement TCP server. `Listener.OnAccept` will establish incomming connection by `Connection`. `Connector` is used to implement TCP client.

`Connector` and `Connection` are a pair of TCP stream, implement `OnRecv`, `OnClose` by yourself. `Enqueue` is used to enqueue output data.

`Peer` is used to implement UDP application. Implement `OnRecv` by yourself.

## Examples

Refer [examples](./examples/)

1. TCP server and client:

   [Echo Server](./examples/echo_server.cc) and [Echo Client](./examples/echo_client.cc)

2. UDP pairs:

   [UDP Server](./examples/peer_s.cc) and [UDP Client](./examples/pees_c.cc)

3. Chat server:

   [Chat Server](./examples/chat_server.cc) and [Chat Client](./examples/chat_client.cc)
