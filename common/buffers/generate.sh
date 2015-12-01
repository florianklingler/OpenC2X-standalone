#!/bin/sh

protoc --proto_path=./ --cpp_out=build/ cam.proto 

protoc --proto_path=./ --cpp_out=build/ denm.proto 


