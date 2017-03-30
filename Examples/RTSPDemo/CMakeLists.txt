PROJECT(RTSPDemo)

cmake_minimum_required(VERSION 2.4)
if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

find_package(OpenIGTLink REQUIRED)

include(${OpenIGTLink_USE_FILE})

ADD_EXECUTABLE(RTSPClientDemo   RTSPClientDemo.cpp)
TARGET_LINK_LIBRARIES(RTSPClientDemo OpenIGTLink)

ADD_EXECUTABLE(RTSPServerDemo   RTSPServerDemo.cpp)
TARGET_LINK_LIBRARIES(RTSPServerDemo OpenIGTLink)



