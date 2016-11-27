SET(CMAKE_CXX_FLAGS "-g -Wall -std=c++11")

include_directories(
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/httpServer/src/external/
)
set(httpServer_SRCS
   httpServer.cpp
   external/pbjson.cpp
   external/pbjson.hpp
)

add_executable (httpServer ${httpServer_SRCS} ${pbjson_SRCS})
target_link_libraries (httpServer zmq protobuf boost_system boost_thread pthread asn proto messages utility)

ADD_CUSTOM_TARGET(
    httpServer-config ALL COMMENT "Installing Recource-Files into Builddirectory"
)

ADD_CUSTOM_COMMAND(
    TARGET httpServer-config
    COMMAND mkdir -p ${CMAKE_BINARY_DIR}/binaries/httpServer/config/
    COMMAND mkdir -p ${CMAKE_BINARY_DIR}/binaries/httpServer/bin/
    COMMAND cp -a ${CMAKE_CURRENT_SOURCE_DIR}/../config/* ${CMAKE_BINARY_DIR}/binaries/httpServer/config/
    COMMAND cp -a ${CMAKE_BINARY_DIR}/httpServer/src/httpServer ${CMAKE_BINARY_DIR}/binaries/httpServer/bin/
)

