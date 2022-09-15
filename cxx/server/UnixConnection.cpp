#include "UnixConnection.h"

std::shared_ptr<UnixConnection> UnixConnection::create(boost::asio::io_context& io, std::shared_ptr<Exchange>& xch) {
    return std::make_shared<UnixConnection>(io, xch);
}

void UnixConnection::handle_read(const boost::system::error_code& error, size_t num_bytes_transferred) {
    if (error) {
        std::cout << __func__ << " got error: " << error.what() << std::endl;
    } else {
        std::string_view view = std::string_view(read_message, num_bytes_transferred);
        std::cout << __func__ << " rcvd: " << view << std::endl;
    }

    // stop calling yourself!
    boost::asio::async_read(
        socket,
        boost::asio::buffer(read_message),
        boost::asio::transfer_at_least(4),
        [this] (const boost::system::error_code& e, size_t nbt) {
            this->handle_read(e, nbt);
        }
    );
}