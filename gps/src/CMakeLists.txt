SET(CMAKE_CXX_FLAGS "-g -Wall -std=c++11")

set(gps_SRCS
   GpsService.cpp
)

add_executable (gpsService ${gps_SRCS})
target_link_libraries (gpsService zmq protobuf boost_system boost_thread gps asn proto messages utility)

ADD_CUSTOM_TARGET(
    gpsService-config ALL COMMENT "Installing Recource-Files into Builddirectory"
)

ADD_CUSTOM_COMMAND(
    TARGET gpsService-config
    COMMAND mkdir -p ${CMAKE_BINARY_DIR}/binaries/gps/config/
    COMMAND mkdir -p ${CMAKE_BINARY_DIR}/binaries/gps/gpsdata/
    COMMAND mkdir -p ${CMAKE_BINARY_DIR}/binaries/gps/bin/
    COMMAND cp -a ${CMAKE_CURRENT_SOURCE_DIR}/../config/* ${CMAKE_BINARY_DIR}/binaries/gps/config/
    COMMAND cp -a ${CMAKE_CURRENT_SOURCE_DIR}/../gpsdata/* ${CMAKE_BINARY_DIR}/binaries/gps/gpsdata/
    COMMAND cp -a ${CMAKE_BINARY_DIR}/gps/src/gpsService ${CMAKE_BINARY_DIR}/binaries/gps/bin/
)

