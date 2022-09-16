#include "PriceLevel.h"
#include "../server/UnixConnection.h"

PriceLevel::PriceLevel(const std::shared_ptr<Order>& order) {
    orders.emplace_front(order);
    total_qty = order->get_qty_remaining();
}

// add an order to the BACK of the queue
void PriceLevel::add_order(const std::shared_ptr<Order>& order) {
    orders.emplace_back(order);
    total_qty += order->get_qty_remaining();
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
        total_qty -= order->get_qty_remaining();
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

bool PriceLevel::consume(const std::shared_ptr<Order>& market) {
    if (empty()) {
        std::cout << "smth is fucked -- called consume on empty price level" << std::endl;
        return false;
    }
    auto limit = get_top_order();

    if (market->get_side() == Side::BUY) {
        auto buyer = market->get_account();
        auto seller = limit->get_account();
        // order verification (eg checking balance) would occur here
        size_t limit_qty = limit->get_qty_remaining();
        size_t market_qty = market->get_qty_remaining();
        size_t fill_qty = std::min(limit_qty, market_qty);
        // populate fill_to_notify objects here
        if (market_qty > limit_qty) {
            orders.pop_front();
            market->partially_fill(fill_qty);
            total_qty -= fill_qty;
        } else if (market_qty < limit_qty) {
            limit->partially_fill(fill_qty);
            market->partially_fill(fill_qty);
            total_qty -= fill_qty;
        } else {
            orders.pop_front();
            limit->partially_fill(fill_qty);
            market->partially_fill(fill_qty);
            total_qty -= fill_qty;
        }

        // update account balances (use account pointers in orders??)
        double tx_val = limit->get_price().get() * fill_qty;
        buyer->debit(tx_val);
        seller->credit(tx_val);

        // update account positions
        size_t iid = market->get_instrument_id();
        buyer->set_position(iid, buyer->get_position(iid) + fill_qty);
        seller->set_position(iid, seller->get_position(iid) - fill_qty);
        
        // notify fills
        if (limit->connection) {
            limit->connection->notify_fill(
                iid, 
                fill_qty, 
                limit->get_price(), 
                buyer->get_account_name(), 
                seller->get_account_name(),
                Side::SELL,
                buyer->get_position(iid),
                seller->get_position(iid)
            );
        }
        

        return true;
    } else {
        auto seller = market->get_account();
        auto buyer = limit->get_account();
        size_t limit_qty = limit->get_qty_remaining();
        size_t market_qty = market->get_qty_remaining();
        size_t fill_qty = std::min(limit_qty, market_qty);

        if (market_qty > limit_qty) {
            orders.pop_front();
            market->partially_fill(fill_qty);
            total_qty -= fill_qty;
        } else if (market_qty < limit_qty) {
            limit->partially_fill(fill_qty);
            market->partially_fill(fill_qty);
            total_qty -= fill_qty;
        } else {
            orders.pop_front();
            limit->partially_fill(fill_qty);
            market->partially_fill(fill_qty);
            total_qty -= fill_qty;
        }

        // update account balances (use account pointers in orders??)
        double tx_val = limit->get_price().get() * fill_qty;
        buyer->debit(tx_val);
        seller->credit(tx_val);

        // update account positions
        size_t iid = market->get_instrument_id();
        buyer->set_position(iid, buyer->get_position(iid) + fill_qty);
        seller->set_position(iid, seller->get_position(iid) - fill_qty);

        // notify fills
        if (limit->connection) {
            limit->connection->notify_fill(
                iid, 
                fill_qty, 
                limit->get_price(), 
                buyer->get_account_name(), 
                seller->get_account_name(),
                Side::BUY,
                buyer->get_position(iid),
                seller->get_position(iid)
            );
        }

        return true;
    }
}