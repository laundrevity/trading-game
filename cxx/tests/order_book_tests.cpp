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

TEST(OrderBookTests, PartialCrossingInsert) {
    auto book = OrderBook(0);
    auto bid_order_1 = std::make_shared<Order>(0, "conor", 0, Price(100), 100, Side::BUY);
    auto bid_order_2 = std::make_shared<Order>(2, "conor", 0, Price(155), 50, Side::BUY);
    auto ask_order = std::make_shared<Order>(1, "conor", 0, Price(150), 100, Side::SELL);

    ASSERT_TRUE(book.insert_order(bid_order_1));
    ASSERT_TRUE(book.insert_order(ask_order));
    ASSERT_EQ(book.get_top_bid(), bid_order_1->get_price());
    ASSERT_EQ(book.get_top_ask(), ask_order->get_price());

    ASSERT_TRUE(book.insert_order(bid_order_2));
    ASSERT_EQ(book.get_top_bid(), bid_order_1->get_price());
    ASSERT_EQ(book.get_top_ask(), ask_order->get_price());
    int ask_px_int = ask_order->get_price().get_int();
    auto top_ask_level = book.get_ask_levels()[ask_px_int];
    ASSERT_EQ(top_ask_level->get_total_qty(), 50);
}

TEST(OrderBookTests, ReplaceCrossingInsert) {
    auto book = OrderBook(0);
    auto bid_order_1 = std::make_shared<Order>(0, "conor", 0, Price(100), 100, Side::BUY);
    auto bid_order_2 = std::make_shared<Order>(2, "conor", 0, Price(155), 150, Side::BUY);
    auto ask_order = std::make_shared<Order>(1, "conor", 0, Price(150), 100, Side::SELL);

    ASSERT_TRUE(book.insert_order(bid_order_1));
    ASSERT_TRUE(book.insert_order(ask_order));
    ASSERT_EQ(book.get_top_bid(), bid_order_1->get_price());
    ASSERT_EQ(book.get_top_ask(), ask_order->get_price());

    ASSERT_TRUE(book.insert_order(bid_order_2));
    ASSERT_EQ(book.get_top_bid(), bid_order_2->get_price());
    ASSERT_EQ(book.get_top_ask(), Price(Limit::MAX));
    int bid_px_int = bid_order_2->get_price().get_int();
    auto top_bid_level = book.get_bid_levels()[bid_px_int];
    ASSERT_EQ(top_bid_level->get_total_qty(), 50);
}

TEST(OrderBookTests, ImproveBestAsk) {
    auto book = OrderBook(0);
    auto ask_order_1 = std::make_shared<Order>(0, "conor", 0, Price(10500), 1, Side::SELL);
    auto ask_order_2 = std::make_shared<Order>(1, "conor", 0, Price(10000), 1, Side::SELL);
    
    ASSERT_TRUE(book.insert_order(ask_order_1));
    ASSERT_TRUE(book.insert_order(ask_order_2));
}