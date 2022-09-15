#pragma once

#include "UnixConnection.h"

class GameServer : public std::enable_shared_from_this<GameServer> {
public:
    GameServer(boost::asio::io_context& io, std::shared_ptr<Exchange>& exchange);
    void start_accept();
private:
    void handle_accept(const std::shared_ptr<UnixConnection>& unix_con, const boost::system::error_code& error);

    boost::asio::io_context& io;
    std::shared_ptr<Exchange> exchange;
    boost::asio::local::stream_protocol::acceptor unix_acceptor;
    std::shared_ptr<UnixConnection> web_server;
    bool unix_connected{false};
};