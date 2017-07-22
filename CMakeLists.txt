cmake_minimum_required(VERSION 2.8)

project(openc2x C CXX)

SET(PROJECT_VERSION_MAJOR "1")
SET(PROJECT_VERSION_MINOR "0")
SET(CMAKE_CXX_FLAGS "-g -Wall -std=c++11")

find_package(Boost COMPONENTS date_time system thread)
find_package(Threads REQUIRED)

include_directories(
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/common/asn1/
    ${PROJECT_SOURCE_DIR}/common/utility/external/
    ${PROJECT_SOURCE_DIR}/common/config/
)

link_directories(
    ${Boost_LIBRARY_DIRS}
    ${ASN1C_EXECUTABLE}	
)

add_subdirectory(common)
add_subdirectory(dcc)
add_subdirectory(cam)
add_subdirectory(denm)
add_subdirectory(denmApp)
add_subdirectory(gps)
add_subdirectory(obd2)
add_subdirectory(ldm)
add_subdirectory(httpServer)


ADD_CUSTOM_TARGET(
    config ALL COMMENT "Installing Recource-Files into Builddirectory"
)

ADD_CUSTOM_COMMAND(
    TARGET config
    COMMAND mkdir -p ${CMAKE_BINARY_DIR}/binaries
)

