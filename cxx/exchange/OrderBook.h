#pragma once
#include <map>
#include "PriceLevel.h"
#include <optional>

class OrderBook {
public:
    OrderBook(size_t instrument_id_in, Price settle);
    OrderBook(size_t instrument_id_in);

    bool cancel_order(size_t order_id);

    // this first checks for fill, and calls append_order if no fill
    bool insert_order(const std::shared_ptr<Order>& order);

    // this doesnt check for fill, just will add it 
    void append_order(const std::shared_ptr<Order>& order);

    size_t get_instrument_id() const {
        return instrument_id;
    }

    std::map<int, std::shared_ptr<PriceLevel>> get_bid_levels() const {
        return bid_levels;
    }

    std::map<int, std::shared_ptr<PriceLevel>> get_ask_levels() const {
        return ask_levels;
    }

    std::shared_ptr<PriceLevel> get_price_level(int price_int);

    Price get_top_bid() const { 
        return best_bid; 
    }
    Price get_top_ask() const { 
        return best_ask; 
    }

    void recalculate_top_bid();
    void recalculate_top_ask();

    std::optional<std::shared_ptr<Order>> get_order(size_t oid);

    std::map<size_t, std::shared_ptr<Order>> get_order_map() {
        return orders;
    }

private:
    size_t instrument_id;
    std::map<int, std::shared_ptr<PriceLevel>> bid_levels;
    std::map<int, std::shared_ptr<PriceLevel>> ask_levels;
    std::map<size_t, std::shared_ptr<Order>> orders;
    // make sure to always update best_bid, best_ask, if necessary
    // whenever inserting or cancelling orders
    Price best_bid{Limit::MIN};
    Price best_ask{Limit::MAX};
    Price settlement_value;
};