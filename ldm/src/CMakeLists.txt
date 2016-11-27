SET(CMAKE_CXX_FLAGS "-g -Wall -std=c++11")

set(ldm_SRCS
   ldm.cpp
)

add_executable (ldm ${ldm_SRCS})
target_link_libraries (ldm zmq protobuf sqlite3 boost_system boost_thread asn proto messages utility)

ADD_CUSTOM_TARGET(
    ldm-config ALL COMMENT "Installing Recource-Files into Builddirectory"
)

ADD_CUSTOM_COMMAND(
    TARGET ldm-config
    COMMAND mkdir -p ${CMAKE_BINARY_DIR}/binaries/ldm/config/
    COMMAND mkdir -p ${CMAKE_BINARY_DIR}/binaries/ldm/bin/
    COMMAND mkdir -p ${CMAKE_BINARY_DIR}/binaries/ldm/db/
    COMMAND cp -a ${CMAKE_CURRENT_SOURCE_DIR}/../config/* ${CMAKE_BINARY_DIR}/binaries/ldm/config/
    COMMAND cp -a ${CMAKE_BINARY_DIR}/ldm/src/ldm ${CMAKE_BINARY_DIR}/binaries/ldm/bin/
)

