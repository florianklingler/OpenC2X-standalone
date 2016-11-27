SET(CMAKE_CXX_FLAGS "-g -Wall -std=c++11")

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

ADD_CUSTOM_TARGET(
    dcc-config ALL COMMENT "Installing Recource-Files into Builddirectory"
)

ADD_CUSTOM_COMMAND(
    TARGET dcc-config
    COMMAND mkdir -p ${CMAKE_BINARY_DIR}/binaries/dcc/config/
    COMMAND mkdir -p ${CMAKE_BINARY_DIR}/binaries/dcc/bin/
    COMMAND cp -a ${CMAKE_CURRENT_SOURCE_DIR}/../config/* ${CMAKE_BINARY_DIR}/binaries/dcc/config/
    COMMAND cp -a ${CMAKE_BINARY_DIR}/dcc/src/dcc ${CMAKE_BINARY_DIR}/binaries/dcc/bin/
)

