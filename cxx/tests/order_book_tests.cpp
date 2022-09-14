#include <gtest/gtest.h>
#include "OrderBook.h"

TEST(OrderBookTests, SimpleAppend) {
    auto book = OrderBook(0);
    auto bid_order = std::make_shared<Order>(0, "conor", 0, Price(100), 100, Side::BUY);
    auto ask_order = std::make_shared<Order>(1, "conor", 0, Price(150), 100, Side::SELL);
    
    ASSERT_EQ(book.get_top_bid(), Price(Limit::MIN));
    ASSERT_EQ(book.get_top_ask(), Price(Limit::MAX));

    book.append_order(bid_order);
    ASSERT_EQ(book.get_top_bid(), bid_order->get_price());
    
    book.append_order(ask_order);
    ASSERT_EQ(book.get_top_ask(), ask_order->get_price());
}

TEST(OrderBookTests, SimpleCancel) {
    auto book = OrderBook(0);
    auto bid_order = std::make_shared<Order>(0, "conor", 0, Price(100), 100, Side::BUY);
    auto ask_order = std::make_shared<Order>(1, "conor", 0, Price(150), 100, Side::SELL);
    book.append_order(bid_order);
    book.append_order(ask_order);

    ASSERT_TRUE(book.cancel_order(0));
    ASSERT_EQ(book.get_top_bid(), Price(Limit::MIN));
    
    ASSERT_TRUE(book.cancel_order(1));
    ASSERT_EQ(book.get_top_ask(), Price(Limit::MAX));
}

TEST(OrderBookTests, SimpleInsert) {
    auto book = OrderBook(0);
    auto bid_order = std::make_shared<Order>(0, "conor", 0, Price(100), 100, Side::BUY);
    ASSERT_TRUE(book.insert_order(bid_order));
    ASSERT_EQ(book.get_top_bid(), bid_order->get_price());
    
    auto ask_order = std::make_shared<Order>(1, "conor", 0, Price(150), 100, Side::SELL);
    ASSERT_TRUE(book.insert_order(ask_order));
    ASSERT_EQ(book.get_top_ask(), ask_order->get_price());
}