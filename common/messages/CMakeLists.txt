set(messages_SRCS
   MessageUtils.cpp
)

add_library(messages SHARED ${messages_SRCS})
target_link_libraries(messages asn)
install(TARGETS messages DESTINATION lib)
