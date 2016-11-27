SET(CMAKE_CXX_FLAGS "-g -Wall -std=c++11")

set(denm_app_SRCS
   denmApp.cpp
)

add_executable (denmApp ${denm_app_SRCS})
target_link_libraries (denmApp zmq protobuf boost_system boost_thread asn proto messages utility)

ADD_CUSTOM_TARGET(
    denmApp-config ALL COMMENT "Installing Recource-Files into Builddirectory"
)

ADD_CUSTOM_COMMAND(
    TARGET denmApp-config
    COMMAND mkdir -p ${CMAKE_BINARY_DIR}/binaries/denmApp/config/
    COMMAND mkdir -p ${CMAKE_BINARY_DIR}/binaries/denmApp/bin/
    COMMAND cp -a ${CMAKE_CURRENT_SOURCE_DIR}/../config/* ${CMAKE_BINARY_DIR}/binaries/denmApp/config/
    COMMAND cp -a ${CMAKE_BINARY_DIR}/denmApp/src/denmApp ${CMAKE_BINARY_DIR}/binaries/denmApp/bin/
)

