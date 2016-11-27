SET(CMAKE_CXX_FLAGS "-g -Wall -Werror -std=c++11")
add_subdirectory(asn1)
add_subdirectory(buffers)
add_subdirectory(messages)
add_subdirectory(utility)

ADD_CUSTOM_TARGET(
    common-config ALL COMMENT "Installing Recource-Files into Builddirectory"
)

ADD_CUSTOM_COMMAND(
    TARGET common-config
    COMMAND mkdir -p ${CMAKE_BINARY_DIR}/binaries/common/config/
    COMMAND cp -a ${CMAKE_CURRENT_SOURCE_DIR}/config/config.xml ${CMAKE_BINARY_DIR}/binaries/common/config/
)

