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

    void write(const std::string& message);
    // use the overload that takes a message to write stuff
    void write();

    void handle_write(const boost::system::error_code& e, size_t nbt);

    void handle_read(const boost::system::error_code& e, size_t nbt);

    char read_message[1024]{};

private:
    boost::asio::local::stream_protocol::socket socket;
    std::shared_ptr<Exchange>& exchange;
    // JL: send buffers are _NOT_ thread safe. use locks or smth if using multithreading
    std::vector<char> send_buffer_;
    std::vector<char> waiting_to_be_sent_;

};