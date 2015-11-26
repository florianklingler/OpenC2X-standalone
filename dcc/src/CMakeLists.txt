cmake_minimum_required (VERSION 2.6)

project (skeleton-dcc)

#http://zeromq.org/bindings:c
#FIND_LIBRARY(ZMQ_LIB ZeroMQ)

add_executable (skeleton-dcc dcc.cpp main.cpp)
TARGET_LINK_LIBRARIES(skeleton-dcc zmq)
