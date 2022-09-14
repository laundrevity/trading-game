#include <gtest/gtest.h>
#include "../exchange/Price.h"

TEST(PriceTests, MinMaxRegular) {
    auto p_max = Price(Limit::MAX);
    auto p_min = Price(Limit::MIN);
    auto p_reg = Price(100);

    ASSERT_TRUE(p_reg < p_max);
    ASSERT_TRUE(p_min < p_reg);
    ASSERT_TRUE(p_min < p_max);
    
    ASSERT_TRUE(p_max > p_reg);
    ASSERT_TRUE(p_reg > p_min);
    ASSERT_TRUE(p_max > p_min);
}