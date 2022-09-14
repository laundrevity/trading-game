#include <gtest/gtest.h>
#include "../exchange/PriceLevel.h"

TEST(PriceLevelTests, AddOrder) {
    auto order1 = std::make_shared<Order>(0, "conor", Price(100), 100, Side::BUY);
    auto level = PriceLevel(order1);
    ASSERT_EQ(level.get_total_qty(), order1->get_qty_remaining());

    auto order2 = std::make_shared<Order>(1, "conor", Price(100), 100, Side::BUY);
    level.add_order(order2);
    ASSERT_EQ(level.get_total_qty(), order1->get_qty_remaining() + order2->get_qty_remaining());
}

TEST(PriceLevelTests, CancelOrder) {
    auto order1 = std::make_shared<Order>(0, "conor", Price(100), 100, Side::BUY);
    auto level = PriceLevel(order1);
    ASSERT_EQ(level.get_total_qty(), order1->get_qty_remaining());

    auto order2 = std::make_shared<Order>(1, "conor", Price(100), 100, Side::BUY);
    level.add_order(order2);
    ASSERT_EQ(level.get_total_qty(), order1->get_qty_remaining() + order2->get_qty_remaining());

    ASSERT_TRUE(level.cancel_order(0));
    ASSERT_EQ(level.get_total_qty(), order2->get_qty_remaining());
}