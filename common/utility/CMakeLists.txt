set(utiltiy_SRCS
   CommunicationClient.cpp
   CommunicationReceiver.cpp
   CommunicationSender.cpp
   CommunicationServer.cpp
   LoggingUtility.cpp
   Utils.cpp

)

add_library(utility SHARED ${utiltiy_SRCS})
#target_link_libraries(messages boost)
install(TARGETS utility DESTINATION lib)
