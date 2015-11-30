Compile Google Protocol Buffers:
protoc -I=./ --cpp_out=./ msgs.proto
Generates .pb.h and .pb.cc files for serialization. Just include the .pb.h file to use them.

Compile a module (with main):
g++ module.cpp ../../lib/msgs.pb.cc -o module -lzmq -lprotobuf
