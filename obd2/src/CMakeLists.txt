set(obd2_SRCS
   Obd2Service.cpp
   SerialPort.cpp
)

add_executable (obd2 ${obd2_SRCS})
target_link_libraries (obd2 pthread zmq protobuf boost_system boost_thread asn proto messages utility uci)

install(TARGETS obd2 DESTINATION bin)
