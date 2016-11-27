SET(CMAKE_CXX_FLAGS "-g -Wall -std=c++11")

set(denm_SRCS
   denservice.cpp
)

add_executable (denm ${denm_SRCS})
target_link_libraries (denm zmq protobuf boost_system boost_thread asn proto messages utility)

ADD_CUSTOM_TARGET(
    denm-config ALL COMMENT "Installing Recource-Files into Builddirectory"
)

ADD_CUSTOM_COMMAND(
    TARGET denm-config
    COMMAND mkdir -p ${CMAKE_BINARY_DIR}/binaries/denm/config/
    COMMAND mkdir -p ${CMAKE_BINARY_DIR}/binaries/denm/bin/
    COMMAND cp -a ${CMAKE_CURRENT_SOURCE_DIR}/../config/* ${CMAKE_BINARY_DIR}/binaries/denm/config/
    COMMAND cp -a ${CMAKE_BINARY_DIR}/denm/src/denm ${CMAKE_BINARY_DIR}/binaries/denm/bin/
)

