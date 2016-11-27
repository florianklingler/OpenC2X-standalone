SET(CMAKE_CXX_FLAGS "-g -Wall -std=c++11")

set(cam_SRCS
   caservice.cpp
)

add_executable (cam ${cam_SRCS})
target_link_libraries (cam zmq protobuf boost_system boost_thread asn proto messages utility)

ADD_CUSTOM_TARGET(
    cam-config ALL COMMENT "Installing Recource-Files into Builddirectory"
)

ADD_CUSTOM_COMMAND(
    TARGET cam-config
    COMMAND mkdir -p ${CMAKE_BINARY_DIR}/binaries/cam/config/
    COMMAND mkdir -p ${CMAKE_BINARY_DIR}/binaries/cam/bin/
    COMMAND cp -a ${CMAKE_CURRENT_SOURCE_DIR}/../config/* ${CMAKE_BINARY_DIR}/binaries/cam/config/
    COMMAND cp -a ${CMAKE_BINARY_DIR}/cam/src/cam ${CMAKE_BINARY_DIR}/binaries/cam/bin/
)

