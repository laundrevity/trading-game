#pragma once
#include "OrderBook.h"
#include "Account.h"

class Exchange {
public:
    Exchange() = default;
    void register_account(const std::shared_ptr<Account>& account);
    void register_book(const std::shared_ptr<OrderBook>& book);

    std::optional<std::shared_ptr<Account>> get_account(std::string account_name);
    std::optional<std::shared_ptr<OrderBook>> get_book(size_t instrument_id);
    

    bool process_order(const std::shared_ptr<Order>& order);
    bool cancel_order(size_t book_id, size_t order_id);


    std::map<size_t, double> get_nlv_map();

    std::map<size_t, std::shared_ptr<OrderBook>> get_book_map() const {
        return book_map;
    }

private:
    std::map<std::string, std::shared_ptr<Account>> account_map;
    std::map<size_t, std::shared_ptr<OrderBook>> book_map;
    std::map<size_t, size_t> book_order_ids; 
};