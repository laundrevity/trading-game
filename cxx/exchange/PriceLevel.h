#pragma once
#include "Order.h"
#include <deque>
#include <memory>

class PriceLevel {
public:
    // creates a new price level from the given order
    PriceLevel(const std::shared_ptr<Order>& order);

    void add_order(const std::shared_ptr<Order>& order);

    bool cancel_order(size_t order_id);

    bool empty() const {
        return orders.empty();
    }

    std::shared_ptr<Order> get_top_order() const;

    int get_total_qty() const {
        return total_qty;
    }

private:
    std::deque<std::shared_ptr<Order>> orders;
    int total_qty;
};