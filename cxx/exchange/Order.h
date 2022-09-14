#pragma once
#include "Account.h"
#include "Price.h"

enum class Side{
    BUY = 0,
    SELL = 1
};

class Order {
public:
    Order(size_t order_id, std::string account_name, Price price, size_t qty, Side side);

    size_t get_order_id() const {
        return order_id;
    }
    
    size_t get_qty_remaining() const {
        return initial_qty - filled_qty;
    }

    Side get_side() const {
        return side;
    }

private:
    size_t order_id;
    std::string account_name;
    Price price;
    size_t initial_qty;
    size_t filled_qty;
    Side side;
    bool alive;
};