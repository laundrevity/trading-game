#include "PriceLevel.h"

PriceLevel::PriceLevel(const std::shared_ptr<Order>& order) {
    orders.emplace_front(order);
    total_qty = order->get_qty_remaining() * (order->get_side() == Side::BUY ? 1 : -1);
}

// add an order to the BACK of the queue
void PriceLevel::add_order(const std::shared_ptr<Order>& order) {
    orders.emplace_back(order);
    total_qty += order->get_qty_remaining() * (order->get_side() == Side::BUY ? 1 : -1);
}

bool PriceLevel::cancel_order(size_t order_id) {
    bool found = false;
    auto it = orders.begin();
    while ((it != orders.end()) and (not found)) {
        auto level_order = *it;
        if (level_order->get_order_id() == order_id) {
            found = true;
        }
        ++it;
    }

    if (found) {
        --it;
        auto order = *it;
        orders.erase(it);
        total_qty -= order->get_qty_remaining() * (order->get_side() == Side::BUY ? 1 : -1);
        std::cout << "found and cancelled order" << std::endl;
        return true;
    } else {
        std::cout << "did not find order w/ order_id=" << order_id << std::endl;
        return false;
    }
}

std::shared_ptr<Order> PriceLevel::get_top_order() const {
    if (empty()) {
        return nullptr;
    }
    return *(orders.begin());
}