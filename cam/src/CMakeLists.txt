set(cam_SRCS
   caservice.cpp
)

add_executable (cam ${cam_SRCS})
target_link_libraries (cam zmq protobuf boost_system boost_thread asn proto messages utility)
