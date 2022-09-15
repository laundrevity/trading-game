#include "../server/GameServer.h"

int main() {

    ::unlink("/app/sock");

    boost::asio::io_context io;
    auto exchange = std::make_shared<Exchange>();
    auto game_server = std::make_shared<GameServer>(io, exchange);
    io.run();

    return 0;
}