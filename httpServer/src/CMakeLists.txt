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
