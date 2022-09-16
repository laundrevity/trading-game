#include <gtest/gtest.h>
#include "../exchange/PriceLevel.h"

TEST(PriceLevelTests, AddOrder) {
    auto account = std::make_shared<Account>("conor", 0.0);
    auto order1 = std::make_shared<Order>(0, account, 0, Price(100), 100, Side::BUY);
    auto level = PriceLevel(order1);
    ASSERT_EQ(level.get_total_qty(), order1->get_qty_remaining());

    auto order2 = std::make_shared<Order>(1, account, 0, Price(100), 100, Side::BUY);
    level.add_order(order2);
    ASSERT_EQ(level.get_total_qty(), order1->get_qty_remaining() + order2->get_qty_remaining());
}

TEST(PriceLevelTests, CancelOrder) {
    auto account = std::make_shared<Account>("conor", 0.0);
    auto order1 = std::make_shared<Order>(0, account, 0, Price(100), 100, Side::BUY);
    auto level = PriceLevel(order1);
    ASSERT_EQ(level.get_total_qty(), order1->get_qty_remaining());

    auto order2 = std::make_shared<Order>(1, account, 0, Price(100), 100, Side::BUY);
    level.add_order(order2);
    ASSERT_EQ(level.get_total_qty(), order1->get_qty_remaining() + order2->get_qty_remaining());

    ASSERT_TRUE(level.cancel_order(0));
    ASSERT_EQ(level.get_total_qty(), order2->get_qty_remaining());

    ASSERT_FALSE(level.cancel_order(135));
}

TEST(PriceLevelTests, PartialConsume) {
    auto account = std::make_shared<Account>("conor", 0.0);
    auto bid_order = std::make_shared<Order>(0, account, 0, Price(100), 100, Side::BUY);
    auto level = PriceLevel(bid_order);
    ASSERT_EQ(level.get_total_qty(), bid_order->get_qty_remaining());

    auto ask_order = std::make_shared<Order>(1, account, 0, Price(95), 50, Side::SELL);
    ASSERT_TRUE(level.consume(ask_order));
    ASSERT_EQ(level.get_total_qty(), 50);
}

TEST(PriceLevelTests, FullConsume) {
    auto account = std::make_shared<Account>("conor", 0.0);
    auto bid_order = std::make_shared<Order>(0, account, 0, Price(100), 100, Side::BUY);
    auto level = PriceLevel(bid_order);
    ASSERT_EQ(level.get_total_qty(), bid_order->get_qty_remaining());

    auto ask_order = std::make_shared<Order>(1, account, 0, Price(95), 125, Side::SELL);
    ASSERT_TRUE(level.consume(ask_order));
    ASSERT_EQ(level.get_total_qty(), 0);
}