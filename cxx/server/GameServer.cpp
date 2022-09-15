#include "GameServer.h"

GameServer::GameServer(boost::asio::io_context& io_in, std::shared_ptr<Exchange>& exchange_in)
: io(io_in)
, exchange(exchange_in) 
, unix_acceptor(io_in, boost::asio::local::stream_protocol::endpoint("/app/sock")) {
    std::cout << __func__ << std::endl;
    web_server = UnixConnection::create(io_in, exchange_in);
    start_accept();
}

void GameServer::start_accept() {
    if (not unix_connected) {
        unix_acceptor.async_accept(
            web_server->get_socket(),
            [this] (const boost::system::error_code& error) {
                this->handle_accept(web_server, error);
            }  
        );
    }
}

void GameServer::handle_accept(const std::shared_ptr<UnixConnection>& unix_con, const boost::system::error_code& error) {
    if (error) {
        std::cout << "got error=" << error.what() << " in handle_accept" << std::endl;
        start_accept();
    } else {
        unix_connected = true;
        std::cout << __func__ << " connected! starting async_read..." << std::endl;

        // start async read on this connection's socket
        boost::asio::async_read(
            unix_con->get_socket(),
            boost::asio::buffer(unix_con->read_message),
            boost::asio::transfer_at_least(4),
            [unix_con] (const boost::system::error_code& e, size_t nbt) {
                unix_con->handle_read(e, nbt);
            }
        );
    }
}