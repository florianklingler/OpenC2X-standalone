SET(CMAKE_CXX_FLAGS "-g -Wall -std=c++11")

set(obd2_SRCS
   Obd2Service.cpp
   SerialPort.cpp
)

add_executable (obd2 ${obd2_SRCS})
target_link_libraries (obd2 zmq protobuf boost_system boost_thread asn proto messages utility)

ADD_CUSTOM_TARGET(
    obd2-config ALL COMMENT "Installing Recource-Files into Builddirectory"
)

ADD_CUSTOM_COMMAND(
    TARGET obd2-config
    COMMAND mkdir -p ${CMAKE_BINARY_DIR}/binaries/obd2/config/
    COMMAND mkdir -p ${CMAKE_BINARY_DIR}/binaries/obd2/bin/
    COMMAND cp -a ${CMAKE_CURRENT_SOURCE_DIR}/../config/* ${CMAKE_BINARY_DIR}/binaries/obd2/config/
    COMMAND cp -a ${CMAKE_BINARY_DIR}/obd2/src/obd2 ${CMAKE_BINARY_DIR}/binaries/obd2/bin/
)

