#!/bin/sh
mkdir -p build
protoc --proto_path=./ --cpp_out=build/ cam.proto 

protoc --proto_path=./ --cpp_out=build/ denm.proto

protoc --proto_path=./ --cpp_out=build/ data.proto 

protoc --proto_path=./ --cpp_out=build/ gps.proto 

protoc --proto_path=./ --cpp_out=build/ obd2.proto 
