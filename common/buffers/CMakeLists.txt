INCLUDE(FindProtobuf)
FIND_PACKAGE(Protobuf REQUIRED)
INCLUDE_DIRECTORIES(${PROTOBUF_INCLUDE_DIR})
PROTOBUF_GENERATE_CPP(PROTO_SRC PROTO_HEADER BasicContainer.proto BasicVehicleHighFreqContainer.proto BasicVehicleLowFreqContainer.proto camInfo.proto CamParameters.proto cam.proto CoopAwareness.proto data.proto dccInfo.proto DENMessage.proto DENMManagementContainer.proto denm.proto gps.proto HighFreqContainer.proto ItsPduHeader.proto ldmData.proto LowFreqContainer.proto obd2.proto PathPoint.proto ProtectedCommunicationZone.proto RsuHighFreqContainer.proto SpecialVehicleContainer.proto trigger.proto)
ADD_LIBRARY(proto SHARED ${PROTO_HEADER} ${PROTO_SRC})

install(TARGETS proto DESTINATION lib)
