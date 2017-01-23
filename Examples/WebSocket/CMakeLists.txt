PROJECT(WebSocket)

cmake_minimum_required(VERSION 2.4)
if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

find_package(OpenIGTLink REQUIRED)

include(${OpenIGTLink_USE_FILE})


## igtl::WebSocket examples
ADD_EXECUTABLE(WebSocketServer   WebSocketServer.cxx)
TARGET_LINK_LIBRARIES(WebSocketServer OpenIGTLink )
include_directories (${PROJECT_BINARY_DIR}/websocketpp ${PROJECT_BINARY_DIR}/websocketpp/websocketpp)

target_link_libraries(telemetry_server2 ${WEBSOCKETPP_PLATFORM_LIBS} ${Boost_LIBRARIES})

#ADD_EXECUTABLE(WebSocketClient   WebSocketClient.cxx)
#TARGET_LINK_LIBRARIES(WebSocketClient OpenIGTLink)
