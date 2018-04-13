#find_package(NLGENL_LIBRARY REQUIRED HINTS /usr/include/libnl3)


set(dcc_SRCS
   ChannelProber.cpp
   dcc.cpp
   PktStatsCollector.cpp
   ReceiveFromHardwareViaMAC.cpp
   SendToHardwareViaMAC.cpp
)

SET(LIBS dcc pthread zmq protobuf boost_system boost_thread asn proto messages utility uci)

find_package(NL)

IF (NL_FOUND)
	SET(LIBS ${LIBS} ${NL_LIBRARIES})
	include_directories(
		${NL_INCLUDE_DIRS}
	)
ELSE()
	find_package (LIBNL-TINY REQUIRED)
	SET(LIBS ${LIBS} ${LIBNL-TINY_LIBRARIES})
	include_directories(
		${LIBNL-TINY_INCLUDE_DIRS}
	)
ENDIF()

add_executable (dcc ${dcc_SRCS})
target_link_libraries (${LIBS})
install(TARGETS dcc DESTINATION bin)
