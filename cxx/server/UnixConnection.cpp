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

                        case ProtoCommon::INSERT_LIMIT_ORDER: {
                            std::cout << "INSERT_LIMIT_ORDER" << std::endl;
                            
                            auto insert_message = msg.insert_limit_order();
                            auto instrument = insert_message.instrument();
                            size_t instrument_id = instrument.id();
                            size_t precision = instrument.precision();
                            std::string account_name = insert_message.account_name();

                            auto book_opt = exchange->get_book(instrument_id);
                            if (book_opt) {
                                auto account_opt = exchange->get_account(account_name);
                                if (account_opt) {
                                    // create order object
                                    size_t next_order_id = exchange->get_book_order_id(instrument_id);
                                    auto order = std::make_shared<Order>(
                                        next_order_id,
                                        account_opt.value(),
                                        instrument_id,
                                        Price(insert_message.price(), precision),
                                        insert_message.volume(),
                                        (insert_message.side() == ProtoCommon::BUY) ? Side::BUY : Side::SELL
                                    );
                                    order->attach_connection(shared_from_this());

                                    if (exchange->process_order(order)) {
                                        std::cout << "successfully processed order: ";
                                        std::cout << "user=" << account_name << ", px=" << Price(insert_message.price(), precision);
                                        std::cout << ", volume=" << insert_message.volume() << ", side=" << int(order->get_side()) << std::endl;
                                        // send INSERT_ORDER_REPLY with error=0
                                    } else {
                                        std::cout << "failed to process order: ";
                                        std::cout << "user=" << account_name << ", px=" << Price(insert_message.price(), precision);
                                        std::cout << ", volume=" << insert_message.volume() << ", side=" << int(order->get_side()) << std::endl;                                        
                                        // send INSERT_ORDER_REPLY with error=1
                                    }
                                    exchange->increment_book_order_id(instrument_id);
                                } else {
                                    std::cout << "cant send order because account_name=" << account_name << " is not registered" << std::endl;
                                }
                            } else {
                                std::cout << "cant send order because book_id=" << instrument_id << " is not registered" << std::endl;
                            }

                            break;
                        }

                        case ProtoCommon::INSERT_MARKET_ORDER: {
                            std::cout << "INSERT_MARKET_ORDER" << std::endl;
                            
                            auto insert_message = msg.insert_market_order();
                            auto instrument = insert_message.instrument();
                            size_t instrument_id = instrument.id();
                            // size_t precision = instrument.precision();
                            std::string account_name = insert_message.account_name();

                            auto book_opt = exchange->get_book(instrument_id);
                            if (book_opt) {
                                auto account_opt = exchange->get_account(account_name);
                                if (account_opt) {
                                    size_t next_order_id = exchange->get_book_order_id(instrument_id);
                                    auto order = std::make_shared<Order>(
                                        next_order_id,
                                        account_opt.value(),
                                        instrument_id,
                                        insert_message.volume(),
                                        (insert_message.side() == ProtoCommon::BUY) ? Side::BUY : Side::SELL
                                    );
                                    order->attach_connection(shared_from_this());

                                    if (exchange->process_order(order)) {
                                        std::cout << "successfully processed market order: ";
                                        std::cout << "user=" << account_name << ", volume=" << insert_message.volume() << ", side=" << int(order->get_side()) << std::endl;
                                    } else {
                                        std::cout << "failed to process market order";
                                    }
                                    exchange->increment_book_order_id(instrument_id);
                                } else {
                                    std::cout << "cant send order because acct_name=" << account_name << " is not registered" << std::endl;
                                }
                            } else {
                                std::cout << "cant send order because book_id=" << instrument_id << " is not registered" << std::endl;
                            }

                            break;
                        }

                        case ProtoCommon::CANCEL_ORDER: {
                            std::cout << "CANCEL_ORDER" << std::endl;

                            auto cancel = msg.cancel_order();
                            auto instrument = cancel.instrument();
                            size_t instrument_id = instrument.id();
                            size_t precision = instrument.precision();
                            std::string account_name = cancel.account_name();
                            auto price = Price(cancel.price(), precision);
                            auto side = (cancel.side() == ProtoCommon::BUY ? Side::BUY : Side::SELL);

                            auto book_opt = exchange->get_book(instrument_id);
                            if (book_opt) {
                                auto account_opt = exchange->get_account(account_name);
                                if (account_opt) {
                                    auto order_map = book_opt.value()->get_order_map();
                                    for (auto& [oid, order]:  order_map) {
                                        if (order->get_account_name() == account_name) {
                                            if (order->get_price() == price) {
                                                if (order->get_side() == side) {
                                                    book_opt.value()->cancel_order(oid);
                                                }
                                            }
                                        }
                                    }
                                    notify_level_update(instrument_id, account_name, 0, price, side);
                                } else {
                                    std::cout << "failed to cancel order because account=" << account_name << " is unregistered" << std::endl;
                                }
                            } else {
                                std::cout << "failed to cancel order because book=" << instrument_id << " is unregistered" << std::endl;
                            }
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
                            auto settlement_price = Price(create_message.settlement_price());
                            std::cout << "iid=" << instrument_id << ", precision=" << precision << std::endl;

                            // TODO: check to see if the iid already exists in exchange book_map

                            auto book = std::make_shared<OrderBook>(instrument_id, settlement_price);
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

                        case ProtoCommon::CANCEL_USER_SIDE: {
                            std::cout << "CANCEL_USER_SIDE" << std::endl;

                            auto cancel = msg.cancel_user_side();
                            auto instrument = cancel.instrument();
                            size_t instrument_id = instrument.id();
                            std::string account_name = cancel.account_name();
                            auto side = (cancel.side() == ProtoCommon::BUY ? Side::BUY : Side::SELL);

                            auto book_opt = exchange->get_book(instrument_id);
                            if (book_opt) {
                                auto account_opt = exchange->get_account(account_name);
                                if (account_opt) {
                                    auto order_map = book_opt.value()->get_order_map();
                                    for (auto& [oid, order]: order_map) {
                                        if (order->get_account_name() == account_name) {
                                            if (order->get_side() == side) {
                                                std::cout << "canceling oid=" << oid << ", px=" << order->get_price() << std::endl;
                                                book_opt.value()->cancel_order(oid);
                                                notify_level_update(instrument_id, account_name, 0, order->get_price(), side);
                                            }
                                        }
                                    }
                                }
                            }


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

void UnixConnection::notify_fill(
    size_t instrument_id, size_t volume, Price price, std::string buyer, std::string seller, Side passive_side, int buyer_pos, int seller_pos) {
    ProtoCommon::Message trade;
    trade.add_type(ProtoCommon::TRADE);
    trade.mutable_trade()->mutable_instrument()->set_id(instrument_id);
    trade.mutable_trade()->mutable_instrument()->set_precision(price.get_precision());
    trade.mutable_trade()->set_volume(volume);
    trade.mutable_trade()->set_price(price.get_int());
    trade.mutable_trade()->set_buyer(buyer);
    trade.mutable_trade()->set_seller(seller);

    if (passive_side == Side::BUY) {
        trade.mutable_trade()->set_passive_account(buyer);
        trade.mutable_trade()->set_passive_side(ProtoCommon::BUY);
    } else {
        trade.mutable_trade()->set_passive_account(seller);
        trade.mutable_trade()->set_passive_side(ProtoCommon::SELL);
    }

    // trade.mutable_trade()->set_buyer_account(buyer);
    // trade.mutable_trade()->set_seller_account(seller);

    std::string trade_string;
    trade.SerializeToString(&trade_string);
    const auto header = BuildHeader(MessageKind::BUSINESS, trade_string);
    const char* header_bytes = reinterpret_cast<const char*>(&header);
    std::string trade_full_string(header_bytes, 8);
    trade_full_string += trade_string;
    write(trade_full_string);

    // now also write position update messages to each player
    ProtoCommon::Message buyer_update;
    buyer_update.add_type(ProtoCommon::POSITION_UPDATE);
    buyer_update.mutable_position_update()->mutable_instrument()->set_id(instrument_id);
    buyer_update.mutable_position_update()->mutable_instrument()->set_precision(price.get_precision());
    buyer_update.mutable_position_update()->set_account_name(buyer);
    buyer_update.mutable_position_update()->set_position(buyer_pos);
    std::string buyer_string;
    buyer_update.SerializeToString(&buyer_string);
    const auto buyer_header = BuildHeader(MessageKind::BUSINESS, buyer_string);
    const char* buyer_header_bytes = reinterpret_cast<const char*>(&buyer_header);
    std::string buyer_full_string(buyer_header_bytes, 8);
    buyer_full_string += buyer_string;
    write(buyer_full_string);

    ProtoCommon::Message seller_update;
    seller_update.add_type(ProtoCommon::POSITION_UPDATE);
    seller_update.mutable_position_update()->mutable_instrument()->set_id(instrument_id);
    seller_update.mutable_position_update()->mutable_instrument()->set_precision(price.get_precision());
    seller_update.mutable_position_update()->set_account_name(seller);
    seller_update.mutable_position_update()->set_position(seller_pos);
    std::string seller_string;
    seller_update.SerializeToString(&seller_string);
    const auto seller_header = BuildHeader(MessageKind::BUSINESS, seller_string);
    const char* seller_header_bytes = reinterpret_cast<const char*>(&seller_header);
    std::string seller_full_string(seller_header_bytes, 8);
    seller_full_string += seller_string;
    write(seller_full_string);

}

void UnixConnection::notify_level_update(size_t iid, std::string account, size_t volume, Price price, Side side) {
    ProtoCommon::Message update;
    update.add_type(ProtoCommon::LEVEL_UPDATE);
    update.mutable_level_update()->mutable_instrument()->set_id(iid);
    update.mutable_level_update()->mutable_instrument()->set_precision(price.get_precision());\
    update.mutable_level_update()->set_account_name(account);
    update.mutable_level_update()->set_volume(volume);
    update.mutable_level_update()->set_price(price.get_int());
    auto proto_side = (side == Side::BUY ? ProtoCommon::BUY : ProtoCommon::SELL);
    update.mutable_level_update()->set_side(proto_side);

    std::string update_string;
    update.SerializeToString(&update_string);
    const auto header = BuildHeader(MessageKind::BUSINESS, update_string);
    const char* header_bytes = reinterpret_cast<const char*>(&header);
    std::string update_full_string(header_bytes, 8);
    update_full_string += update_string;
    write(update_full_string);
}   