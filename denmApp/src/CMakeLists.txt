set(denm_app_SRCS
   denmApp.cpp
)

add_executable (denmApp ${denm_app_SRCS})
target_link_libraries (denmApp zmq protobuf boost_system boost_thread asn proto messages utility)

