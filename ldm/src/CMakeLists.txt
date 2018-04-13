set(ldm_SRCS
   ldm.cpp
)

add_executable (ldm ${ldm_SRCS})
target_link_libraries (ldm zmq protobuf sqlite3 boost_system boost_thread asn proto messages utility uci)

ADD_CUSTOM_TARGET(
    ldm-db-directory ALL COMMENT "Make db directory"
)

ADD_CUSTOM_COMMAND(
    TARGET ldm-db-directory
    COMMAND mkdir -p ${CMAKE_BINARY_DIR}/ldm/db/
)

install(TARGETS ldm DESTINATION bin)


