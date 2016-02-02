#!/bin/sh
mkdir build
protoc --proto_path=./ --cpp_out=build/ cam.proto 

protoc --proto_path=./ --cpp_out=build/ denm.proto

protoc --proto_path=./ --cpp_out=build/ data.proto 


