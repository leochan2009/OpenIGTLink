PROJECT(LZ4Codec)

cmake_minimum_required(VERSION 2.4)
if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

find_package(OpenIGTLink REQUIRED)

include(${OpenIGTLink_USE_FILE})

ADD_EXECUTABLE(LZ4Client   LZ4Client.cxx)
TARGET_LINK_LIBRARIES(LZ4Client OpenIGTLink)

ADD_EXECUTABLE(LZ4Server   LZ4Server.cxx)
TARGET_LINK_LIBRARIES(LZ4Server OpenIGTLink)
