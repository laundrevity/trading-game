#include <gtest/gtest.h>
#include "../exchange/Exchange.h"

TEST(ExchangeTests, SimpleRegistrationAndInsert) {
    auto exchange = Exchange();
    auto book = std::make_shared<OrderBook>(0);
    auto account = std::make_shared<Account>("conor", 0.0);
    exchange.register_book(book);
    exchange.register_account(account);

    ASSERT_TRUE(exchange.get_book(0));
    ASSERT_TRUE(exchange.get_account("conor"));

    auto order = std::make_shared<Order>(0, account, 0, Price(100), 100, Side::BUY);
    ASSERT_TRUE(exchange.process_order(order));

    auto bad_account = std::make_shared<Account>("ronoc", 0.0);
    auto bad_order = std::make_shared<Order>(1, bad_account, 0, Price(100), 100, Side::BUY);
    ASSERT_FALSE(exchange.process_order(bad_order));
}

TEST(ExchangeTests, PositionOnTrade) {
    auto exchange = Exchange();
    auto book = std::make_shared<OrderBook>(0);
    auto account_1 = std::make_shared<Account>("conor", 0.0);
    auto account_2 = std::make_shared<Account>("ronoc", 0.0);
    exchange.register_book(book);
    exchange.register_account(account_1);
    exchange.register_account(account_2);
    
    ASSERT_EQ(account_1->get_position(0), 0);
    ASSERT_EQ(account_2->get_position(0), 0);

    auto bid = std::make_shared<Order>(0, account_1, 0, Price(100), 100, Side::BUY);
    auto ask = std::make_shared<Order>(1, account_2, 0, Price(100), 100, Side::SELL);
    ASSERT_TRUE(exchange.process_order(bid));
    ASSERT_TRUE(exchange.process_order(ask));

    ASSERT_EQ(account_1->get_position(0), 100);
    ASSERT_EQ(account_2->get_position(0), -100);

}