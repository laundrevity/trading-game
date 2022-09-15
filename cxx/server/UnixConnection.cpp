#include "UnixConnection.h"
#include "ProtoStructs.h"
#include "common.pb.h"

std::shared_ptr<UnixConnection> UnixConnection::create(boost::asio::io_context& io, std::shared_ptr<Exchange>& xch) {
    return std::make_shared<UnixConnection>(io, xch);
}

void UnixConnection::write(const std::string& message) {
    if (send_buffer_.empty()) {
        send_buffer_.insert(send_buffer_.end(), message.begin(), message.end());
        write();
    } else {
        waiting_to_be_sent_.insert(waiting_to_be_sent_.end(), message.begin(), message.end());
    }
}

void UnixConnection::write() {
    boost::asio::async_write(
        socket,
        boost::asio::buffer(send_buffer_.data(), send_buffer_.size()),
        [self = shared_from_this(), this] (const boost::system::error_code& e, size_t nbt) {
            this->handle_write(e, nbt);
        }
    );
}

void UnixConnection::handle_write(const boost::system::error_code& error, size_t num_bytes_transferred) {
    send_buffer_.clear();
    if (not waiting_to_be_sent_.empty()) {
        swap(send_buffer_, waiting_to_be_sent_);
        write();
    }
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

                            auto create_message = msg.create_market();
                            auto instrument = create_message.instrument();
                            size_t instrument_id = instrument.id();
                            size_t precision = instrument.precision();
                            std::cout << "iid=" << instrument_id << ", precision=" << precision << std::endl;

                            // TODO: check to see if the iid already exists in exchange book_map

                            auto book = std::make_shared<OrderBook>(instrument_id);
                            exchange->register_book(book);
                            
                            for (auto& user: create_message.user_name()) {
                                // TODO: check to see if this account already exists, so PnL 
                                // can be shared across rounds
                                std::cout << "user=" << user << std::endl;
                                auto account = std::make_shared<Account>(user, 0.0);
                                exchange->register_account(account);
                            }

                            ProtoCommon::Message reply;
                            reply.add_type(ProtoCommon::CREATE_MARKET_REPLY);
                            reply.mutable_create_market_reply()->mutable_instrument()->set_id(instrument_id);
                            reply.mutable_create_market_reply()->mutable_instrument()->set_precision(precision);
                            std::string reply_string;
                            reply.SerializeToString(&reply_string);
                            const auto header = BuildHeader(MessageKind::BUSINESS, reply_string);
                            const char* header_bytes = reinterpret_cast<const char*>(&header);
                            std::string reply_full_string(header_bytes, 8);
                            reply_full_string += reply_string;
                            write(reply_full_string);

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