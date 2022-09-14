#include "Exchange.h"

void Exchange::register_account(const std::shared_ptr<Account>& account) {
    auto it = account_map.find(account->get_account_name());
    if (it != account_map.end()) {
        std::cout << "already registered account " << account->get_account_name() << std::endl;
        return;
    }
    account_map[account->get_account_name()] = account;
}

void Exchange::register_book(const std::shared_ptr<OrderBook>& book) {
    auto it = book_map.find(book->get_instrument_id());
    if (it != book_map.end()) {
        std::cout << "already registered book for instrument id " << book->get_instrument_id() << std::endl;
        return;
    }
    book_map[book->get_instrument_id()] = book;
    book_order_ids[book->get_instrument_id()] = 0;
}

std::optional<std::shared_ptr<Account>> Exchange::get_account(std::string account_name) {
    auto it = account_map.find(account_name);
    if (it == account_map.end()) {
        return std::nullopt;
    } else {
        return std::make_optional(account_map[account_name]);
    }
}

std::optional<std::shared_ptr<OrderBook>> Exchange::get_book(size_t instrument_id) {
    auto it = book_map.find(instrument_id);
    if (it == book_map.end()) {
        return std::nullopt;
    } else {
        return std::make_optional(book_map[instrument_id]);
    }
}

bool Exchange::process_order(const std::shared_ptr<Order>& order) {
    size_t iid = order->get_instrument_id();
    // ++book_order_ids[iid]; 
    // if not here, then who / what is updating these auto-incrementing order ids? 
    // ExchangeServer?
    
    auto book_cnt = get_book(iid);
    if (not book_cnt) {
        std::cout << "rejecting order due to unregistered instrument id " << iid << std::endl;
        return false;
    }
    auto book = book_cnt.value();

    auto account_cnt = get_account(order->get_account_name());
    if (not account_cnt) {
        std::cout << "rejecting order from unregistered account " << order->get_account_name() << std::endl;
        return false;        
    }

    return book->insert_order(order);
}

bool Exchange::cancel_order(size_t instrument_id, size_t order_id) {
    auto it = book_map.find(instrument_id);
    if (it == book_map.end()) {
        std::cout << "could not cancel order for non-existent iid=" << instrument_id << std::endl;
        return false;        
    }

    auto book = book_map[instrument_id];
    auto order = book->get_order(order_id);
    if (order) {
        return book->cancel_order(order_id);
    } else {
        std::cout << "could not cancel oid=" << order_id << " cuz was not found in book for iid=" << instrument_id << std::endl;
        return false;
    }
}