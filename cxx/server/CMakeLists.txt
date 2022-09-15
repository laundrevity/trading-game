add_library(server_lib UnixConnection.cpp GameServer.cpp ${PROTO_SRCS} ${PROTO_HDRS})

include_directories(server_lib PUBLIC ${Boost_LIBRARIES})