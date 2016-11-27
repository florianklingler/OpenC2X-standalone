INCLUDE(FindProtobuf)
FIND_PACKAGE(Protobuf REQUIRED)
INCLUDE_DIRECTORIES(${PROTOBUF_INCLUDE_DIR})
PROTOBUF_GENERATE_CPP(PROTO_SRC PROTO_HEADER BasicContainer.proto BasicVehicleHighFreqContainer.proto BasicVehicleLowFreqContainer.proto camInfo.proto CamParameters.proto cam.proto CoopAwareness.proto data.proto dccInfo.proto DENMessage.proto DENMManagementContainer.proto denm.proto gps.proto HighFreqContainer.proto ItsPduHeader.proto ldmData.proto LowFreqContainer.proto obd2.proto PathPoint.proto ProtectedCommunicationZone.proto RsuHighFreqContainer.proto SpecialVehicleContainer.proto trigger.proto)
ADD_LIBRARY(proto SHARED ${PROTO_HEADER} ${PROTO_SRC})

install(TARGETS proto DESTINATION lib)

ADD_CUSTOM_TARGET(
    proto-config ALL COMMENT "Installing Recource-Files into Builddirectory"
)

ADD_CUSTOM_COMMAND(
    TARGET proto-config
    COMMAND mkdir -p ${CMAKE_CURRENT_SOURCE_DIR}/build/
    COMMAND cp -a ${CMAKE_BINARY_DIR}/common/buffers/*.cc ${CMAKE_CURRENT_SOURCE_DIR}/build/
    COMMAND cp -a ${CMAKE_BINARY_DIR}/common/buffers/*.h ${CMAKE_CURRENT_SOURCE_DIR}/build/
)

