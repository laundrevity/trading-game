#pragma once

#include <boost/asio.hpp>
#include <memory>
#include "../exchange/Exchange.h"

class UnixConnection : public std::enable_shared_from_this<UnixConnection> {
public:
    UnixConnection(boost::asio::io_context& io, std::shared_ptr<Exchange>& xch)
    : socket(io), exchange(xch) {}
    
    static std::shared_ptr<UnixConnection> create(
        boost::asio::io_context& io,
        std::shared_ptr<Exchange>& exchange);
    
    inline boost::asio::local::stream_protocol::socket& get_socket() {
        return socket;
    }

    char read_message[1024]{};

    void handle_read(const boost::system::error_code& e, size_t nbt);


private:
    boost::asio::local::stream_protocol::socket socket;
    std::shared_ptr<Exchange>& exchange;
};