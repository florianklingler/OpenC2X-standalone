#find_package(NLGENL_LIBRARY REQUIRED HINTS /usr/include/libnl3)

include_directories(
    /usr/include/libnl3
)

set(dcc_SRCS
   ChannelProber.cpp
   dcc.cpp
   PktStatsCollector.cpp
   ReceiveFromHardwareViaMAC.cpp
   SendToHardwareViaMAC.cpp
)

add_executable (dcc ${dcc_SRCS})
target_link_libraries (dcc zmq protobuf boost_system boost_thread nl-3 nl-genl-3 asn proto messages utility)

