#include <gtest/gtest.h>
#include "../exchange/Account.h"

TEST(AccountTests, PositionSetAndCheck) {
    auto acct = Account("conor", 0.0);
    acct.set_position(0, 100);
    ASSERT_EQ(acct.get_position(0), 100);
    acct.set_position(0, -100);
    ASSERT_EQ(acct.get_position(0), -100);
    acct.set_position(1, 30);
    ASSERT_EQ(acct.get_position(1), 30);
}