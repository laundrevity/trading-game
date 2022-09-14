#include "Order.h"

Order::Order(size_t order_id_in, std::string account_name_in, size_t instrument_id_in, Price price_in, size_t qty_in, Side side_in)
: order_id(order_id_in)
, account_name(account_name_in)
, instrument_id(instrument_id_in)
, price(price_in)
, initial_qty(qty_in)
, side(side_in) {
    filled_qty = 0;
    alive = false; // change to true once it gets acknowledged by matcher
}