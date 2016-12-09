PROJECT(SampleUDPProgam)

cmake_minimum_required(VERSION 2.4)
if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

find_package(OpenIGTLink REQUIRED)

include(${OpenIGTLink_USE_FILE})

IF(OpenIGTLink_PLATFORM_WIN32) # for Windows
  SET(LINK_LIBS
    ws2_32
    wsock32
  )
  #For debug under win32 system, the run time check mode should be set to /RTCu for multithreading purpose
  string(REPLACE "/RTC1" "/RTCu" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
  SET(OpenIGTLink_REQUIRED_CXX_FLAGS ${OpenIGTLink_REQUIRED_CXX_FLAGS})
ELSE(OpenIGTLink_PLATFORM_WIN32) # for POSIX-compatible OSs
  SET(LINK_LIBS
    m
    pthread
  )
  IF(OpenIGTLink_PLATFORM_QNX)
    SET(LINK_LIBS
      ${LINK_LIBS}
      c
      socket
    )
  ENDIF(OpenIGTLink_PLATFORM_QNX)
  IF(OpenIGTLink_PLATFORM_SUNOS)
    SET(LINK_LIBS
      ${LINK_LIBS}
      ${OpenIGTLink_STD_LINK_LIBRARIES}
    )
  ENDIF(OpenIGTLink_PLATFORM_SUNOS)
ENDIF(OpenIGTLink_PLATFORM_WIN32)

ADD_EXECUTABLE(ClientUDPTransfer   ClientUDPTransfer.cxx)
TARGET_LINK_LIBRARIES(ClientUDPTransfer ${LINK_LIBS})

ADD_EXECUTABLE(ServerUDPTransfer   ServerUDPTransfer.cxx)
TARGET_LINK_LIBRARIES(ServerUDPTransfer ${LINK_LIBS})



