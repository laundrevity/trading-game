#pragma once
#include "Account.h"
#include "Price.h"
#include <memory>

class UnixConnection;

enum class Side{
    BUY = 0,
    SELL = 1
};

class Order {
public:
    Order(size_t order_id, const std::shared_ptr<Account>& account, size_t instrument_id, Price price, size_t qty, Side side);

    size_t get_order_id() const {
        return order_id;
    }

    std::shared_ptr<Account> get_account() const {
        return account;
    }

    std::string get_account_name() const {
        return account->get_account_name();
    }

    size_t get_instrument_id() const {
        return instrument_id;
    }
    
    Price get_price() const {
        return price;
    }

    size_t get_qty_remaining() const {
        return initial_qty - filled_qty;
    }

    Side get_side() const {
        return side;
    }

    void partially_fill(size_t fill_qty) {
        filled_qty += fill_qty;
    }

    void attach_connection(const std::shared_ptr<UnixConnection>& uc);

private:
    size_t order_id;
    std::shared_ptr<Account> account;
    size_t instrument_id;
    Price price;
    size_t initial_qty;
    size_t filled_qty;
    Side side;
    bool alive;

public:
    std::shared_ptr<UnixConnection> connection;
};