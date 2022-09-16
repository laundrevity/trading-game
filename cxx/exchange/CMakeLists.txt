add_library(exchange_lib
    STATIC
    Price.cpp
    Account.cpp
    Order.cpp
    PriceLevel.cpp
    OrderBook.cpp
    Exchange.cpp)

target_link_libraries(exchange_lib PUBLIC server_lib)