#include "UnixConnection.h"
#include "ProtoStructs.h"
#include "common.pb.h"

std::shared_ptr<UnixConnection> UnixConnection::create(boost::asio::io_context& io, std::shared_ptr<Exchange>& xch) {
    return std::make_shared<UnixConnection>(io, xch);
}

void UnixConnection::handle_read(const boost::system::error_code& error, size_t num_bytes_transferred) {
    if (error) {
        std::cout << __func__ << " got error: " << error.what() << std::endl;
    } else {
        std::string_view view = std::string_view(read_message, num_bytes_transferred);
        std::cout << __func__ << " rcvd: " << view << std::endl;

        if (num_bytes_transferred >= 8) {
            auto bytes = view.substr(0, 8).data();
            Header* header = (Header*) bytes;
            const char* proto_data = view.substr(8, header->size).data();
            ProtoCommon::Message msg;
            bool parse_result = msg.ParseFromArray(proto_data, header->size);
            if (parse_result) {
                for (auto& msg_type: msg.type()) {
                    switch(msg_type) {

                        case ProtoCommon::GENERIC_REPLY: {
                            std::cout << "GENERIC_REPLY" << std::endl;
                            break;
                        }

                        case ProtoCommon::INSERT_ORDER: {
                            std::cout << "INSERT_ORDER" << std::endl;
                            break;
                        }

                        case ProtoCommon::INSERT_ORDER_REPLY: {
                            std::cout << "INSERT_ORDER_REPLY" << std::endl;
                            break;
                        }

                        case ProtoCommon::CANCEL_ORDER: {
                            std::cout << "CANCEL_ORDER" << std::endl;
                            break;
                        }

                        case ProtoCommon::CANCEL_ORDER_REPLY: {
                            std::cout << "CANCEL_ORDER_REPLY" << std::endl;
                            break;
                        }

                        case ProtoCommon::TRADE: {
                            std::cout << "TRADE" << std::endl;
                            break;
                        }

                        case ProtoCommon::CREATE_MARKET: {
                            std::cout << "CREATE_MARKET" << std::endl;
                            break;
                        }
                        
                        default: {
                            std::cout << "Got unknown proto msg type!" << std::endl;
                            break;
                        }
                    }
                }
            }
        }
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