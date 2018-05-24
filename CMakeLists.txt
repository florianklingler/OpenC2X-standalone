cmake_minimum_required(VERSION 2.8)

project(openc2x C CXX)

SET(PROJECT_VERSION_MAJOR "1")
SET(PROJECT_VERSION_MINOR "4")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -std=c++11")
SET(CMAKE_VERBOSE_MAKEFILE "ON")
SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMake/Modules) 
SET(CMAKE_INSTALL_PREFIX "/usr")

find_package(Boost COMPONENTS date_time system thread)
find_package(Threads REQUIRED)

include_directories(
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/common/asn1/
    ${PROJECT_SOURCE_DIR}/common/utility/external/
    ${PROJECT_SOURCE_DIR}/common/config/
    ${PROJECT_BINARY_DIR}
    ${PROJECT_BINARY_DIR}/common/asn1/

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
add_subdirectory(config)
add_subdirectory(scripts)

ADD_CUSTOM_TARGET(
    config ALL COMMENT "Installing Recource-Files into Builddirectory"
)

ADD_CUSTOM_COMMAND(
    TARGET config
    COMMAND mkdir -p ${CMAKE_BINARY_DIR}/binaries
)



SET(CPACK_CMAKE_GENERATOR "Unix Makefiles")
SET(CPACK_SOURCE_GENERATOR "TGZ;TBZ2")
#SET(CPACK_GENERATOR "TGZ;TBZ2;DEB;RPM")
SET(CPACK_GENERATOR "TGZ;TBZ2;DEB")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY " Open Source Experimental and Prototyping Platform Supporting ETSI ITS-G5")
SET(CPACK_PACKAGE_VENDOR "CCS Labs")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
SET(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
SET(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
SET(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}")
SET(CPACK_SOURCE_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${CPACK_PACKAGE_VERSION}")
SET(CPACK_PACKAGE_CONTACT "CCS Labs <info@ccs-labs.org>")
SET(CPACK_PACKAGE_SECTION "devel")
set(CPACK_SET_DESTDIR ON)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "uci, libzmq3, gpsd, libboost-thread-dev")
INCLUDE(CPack)

