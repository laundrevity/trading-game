#include "Order.h"
#include "../server/GameServer.h"

Order::Order(size_t order_id_in, const std::shared_ptr<Account>& account_in, size_t instrument_id_in, Price price_in, size_t qty_in, Side side_in)
: order_id(order_id_in)
, account(account_in)
, instrument_id(instrument_id_in)
, price(price_in)
, initial_qty(qty_in)
, side(side_in) {
    filled_qty = 0;
    alive = false; // change to true once it gets acknowledged by matcher
}

void Order::attach_connection(const std::shared_ptr<UnixConnection>& connection_in) {
    connection = connection_in;
}