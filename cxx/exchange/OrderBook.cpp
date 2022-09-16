#include "OrderBook.h"

OrderBook::OrderBook(size_t instrument_id_in) : instrument_id(instrument_id_in) {}

bool OrderBook::cancel_order(size_t order_id) {
    auto it = orders.find(order_id);
    if (it == orders.end()) {
        std::cout << "tried to cancel oid=" << order_id << " but not in orders map for order book" << std::endl;
        return false;
    }
    auto order = orders[order_id];
    auto order_px_int = order->get_price().get_int();
    if (order->get_side() == Side::BUY) {
        auto level_it = bid_levels.find(order_px_int);
        if (level_it == bid_levels.end()) {
            std::cout << "something is fucked up -- oid=" << order_id << " is in orders, but not bid_levels" << std::endl;
            return false;
        }
        auto level = bid_levels[order_px_int];
        bool result = level->cancel_order(order_id);
        if (result) {
            orders.erase(order->get_order_id());
            // check if this order was in the top level
            if (order->get_price() == best_bid) {
                // check if it was ALONE in the top level
                if (level->empty()) {
                    bid_levels.erase(best_bid.get_int());
                    recalculate_top_bid();
                }
            }
        }
        return result;
    } else {
        auto level_it = ask_levels.find(order_px_int);
        if (level_it == ask_levels.end()) {
            std::cout << "something is fucked up -- oid=" << order_id << " is in orders, but not ask_levels" << std::endl;
            return false;
        }
        auto level = ask_levels[order_px_int];
        bool result = level->cancel_order(order_id);
        if (result) {
            orders.erase(order->get_order_id());
            // check if this order was in the top level
            if (order->get_price() == best_ask) {
                if (level->empty()) {
                    ask_levels.erase(best_ask.get_int());
                    recalculate_top_ask();
                }
            }
        }
        return result;
    }
}

void OrderBook::recalculate_top_bid() {
    if (bid_levels.empty()) {
        best_bid = Price(Limit::MIN);
    } else {
        auto top = Price(Limit::MIN);
        auto it = bid_levels.rbegin();

        // assume that we remove empty levels
        if (it != bid_levels.rend()) {
            top = std::max(top, Price(it->first));
        }
        best_bid = top;
    }
}

void OrderBook::recalculate_top_ask() {
    if (ask_levels.empty()) {
        best_ask = Price(Limit::MAX);
    } else {
        auto top = Price(Limit::MAX);
        auto it = ask_levels.begin();

        // assume that we remove empty levels
        if (it != ask_levels.end()) {
            top = std::min(top, Price(it->first));
        }
        best_ask = top;
    }
}

void OrderBook::append_order(const std::shared_ptr<Order>& order) {
    if (order->get_side() == Side::BUY) {
        if (order->get_price() > best_bid) {
            best_bid = order->get_price();
        }
        int tick_divisor = order->get_price().get_int();
        auto level = bid_levels.find(tick_divisor);

        if (level == bid_levels.end()) {
            bid_levels[tick_divisor] = std::make_shared<PriceLevel>(order);
        } else {
            level->second->add_order(order);
        }
    } else {
        if (order->get_price() < best_ask) {
            best_ask = order->get_price();
        }
        int tick_divisor = order->get_price().get_int();
        auto level = ask_levels.find(tick_divisor);

        if (level == ask_levels.end()) {
            ask_levels[tick_divisor] = std::make_shared<PriceLevel>(order);
        } else {
            level->second->add_order(order);
        }
    }
    orders[order->get_order_id()] = order;
}

// check first for crossing (in which case call PriceLevel::consume)
// if not crossing call append_order
bool OrderBook::insert_order(const std::shared_ptr<Order>& order) {
    bool processing_order = true;
    while(processing_order) {
        if (order->get_side() == Side::BUY) {
            // check if the BUY order is in cross
            if (order->get_price() >= best_ask) {
                auto it = ask_levels.find(best_ask.get_int());
                if (it == ask_levels.end()) {
                    std::cout << "missing price level. ask Conor. bankrupt" << std::endl;
                    return false;
                }

                const auto& top_ask_level = it->second;
                auto top_ask_order = top_ask_level->get_top_order();
                bool success = top_ask_level->consume(order);
                if (not success) {
                    std::cout << "order not filled when it should be!" << std::endl;
                    return false;
                } else {
                    if (top_ask_level->empty()) {
                        ask_levels.erase(it);
                        recalculate_top_ask();
                        // check if order has qty left after depleting level
                        if (order->get_qty_remaining() > 0) {
                            append_order(order);
                            processing_order = false;
                        }
                    }
                }
                if (order->get_qty_remaining() == 0) {
                    processing_order = false;
                }
            // not in cross, so just append and we're chilling 
            } else {
                append_order(order);
                processing_order = false;
            }
        } else {
            // check if the SELL order is in cross
            if (order->get_price() <= best_bid) {
                auto it = bid_levels.find(best_bid.get_int());
                if (it == bid_levels.end()) {
                    std::cout << "missing price level. ask Conor. bankrupt" << std::endl;
                    return false;
                }

                const auto& top_bid_level = it->second;
                auto top_bid_order = top_bid_level->get_top_order();
                bool success = top_bid_level->consume(order);
                if (not success) {
                    std::cout << "order not filled when it should be!" << std::endl;
                    return false;
                } else {
                    if (top_bid_level->empty()) {
                        bid_levels.erase(it);
                        recalculate_top_bid();
                        // check if order has qty left after depleting level
                        if (order->get_qty_remaining() > 0) {
                            append_order(order);
                            processing_order = false;
                        }
                    }
                }
                if (order->get_qty_remaining() == 0) {
                    processing_order = false;
                }
            } else {
                // not in cross, so append and j chilling
                append_order(order);
                processing_order = false;
            }
        }
    }
    return true;
}

std::optional<std::shared_ptr<Order>> OrderBook::get_order(size_t oid) {
    auto it = orders.find(oid);
    if (it == orders.end()) {
        return std::nullopt;
    }

    return std::make_optional(orders[oid]);
}