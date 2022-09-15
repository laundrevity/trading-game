add_library(server_lib UnixConnection.cpp GameServer.cpp)

include_directories(server_lib PUBLIC ${Boost_LIBRARIES})