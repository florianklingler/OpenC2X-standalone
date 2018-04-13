set(gps_SRCS
   GpsService.cpp
)

add_executable (gpsService ${gps_SRCS})
target_link_libraries (gpsService pthread zmq protobuf boost_system boost_thread gps asn proto messages utility uci)

install(TARGETS gpsService DESTINATION bin)
