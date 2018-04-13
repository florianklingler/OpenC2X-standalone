set(denm_SRCS
   denservice.cpp
)

add_executable (denm ${denm_SRCS})
target_link_libraries (denm zmq protobuf boost_system boost_thread asn proto messages utility uci)
install(TARGETS denm DESTINATION bin)
