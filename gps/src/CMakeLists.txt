set(gps_SRCS
   GpsService.cpp
)

add_executable (gpsService ${gps_SRCS})
target_link_libraries (gpsService zmq protobuf boost_system boost_thread gps asn proto messages utility)

