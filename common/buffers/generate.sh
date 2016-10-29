#!/bin/sh
mkdir -p build
cd build
rm *.pb.cc *.pb.h
cd ..

protoc --proto_path=./ --cpp_out=build/ cam.proto 

protoc --proto_path=./ --cpp_out=build/ denm.proto

protoc --proto_path=./ --cpp_out=build/ data.proto 

protoc --proto_path=./ --cpp_out=build/ gps.proto 

protoc --proto_path=./ --cpp_out=build/ obd2.proto 

protoc --proto_path=./ --cpp_out=build/ trigger.proto 

protoc --proto_path=./ --cpp_out=build/ dccInfo.proto 

protoc --proto_path=./ --cpp_out=build/ camInfo.proto 

protoc --proto_path=./ --cpp_out=build/ ldmData.proto 

protoc --proto_path=./ --cpp_out=build/ BasicContainer.proto

protoc --proto_path=./ --cpp_out=build/ BasicVehicleHighFreqContainer.proto

protoc --proto_path=./ --cpp_out=build/ BasicVehicleLowFreqContainer.proto

protoc --proto_path=./ --cpp_out=build/ CamParameters.proto

protoc --proto_path=./ --cpp_out=build/ CoopAwareness.proto

protoc --proto_path=./ --cpp_out=build/ HighFreqContainer.proto

protoc --proto_path=./ --cpp_out=build/ ItsPduHeader.proto

protoc --proto_path=./ --cpp_out=build/ LowFreqContainer.proto

protoc --proto_path=./ --cpp_out=build/ PathPoint.proto

protoc --proto_path=./ --cpp_out=build/ ProtectedCommunicationZone.proto

protoc --proto_path=./ --cpp_out=build/ RsuHighFreqContainer.proto

protoc --proto_path=./ --cpp_out=build/ SpecialVehicleContainer.proto
