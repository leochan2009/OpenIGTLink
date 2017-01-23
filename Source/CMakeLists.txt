# list of libraries
# Note: this part should be moved to OpenIGTLinkConfigPlatform.cmake
IF(OpenIGTLink_PLATFORM_WIN32) # for Windows
  SET(LINK_LIBS
    ws2_32
    wsock32
    )

  #For debug under win32 system, the run time check mode should be set to /RTCu for multithreading purpose
  STRING(REPLACE "/RTC1" "/RTCu" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
  SET(OpenIGTLink_REQUIRED_CXX_FLAGS ${OpenIGTLink_REQUIRED_CXX_FLAGS})
ELSE() # for POSIX-compatible OSs
  SET(LINK_LIBS
    m
    pthread
    )
  IF(OpenIGTLink_PLATFORM_QNX)
    LIST(APPEND LINK_LIBS
      c
      socket
    )
  ENDIF()

  IF(OpenIGTLink_PLATFORM_SUNOS)
    LIST(APPEND LINK_LIBS
      ${OpenIGTLink_STD_LINK_LIBRARIES}
      )
  ENDIF()
ENDIF()

ADD_SUBDIRECTORY( igtlutil )

SET(OpenIGTLink_INCLUDE_DIRS
  PUBLIC
  ${CMAKE_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/igtlutil
  ${CMAKE_CURRENT_BINARY_DIR}/igtlutil
  )

SET(OpenIGTLink_SOURCES
  igtlutil/igtl_header.c
  igtlutil/igtl_image.c
  igtlutil/igtl_transform.c
  igtlutil/igtl_status.c
  igtlutil/igtl_util.c
  igtlutil/igtl_position.c
  igtlutil/igtl_capability.c
  igtlClientSocket.cxx
  igtlCapabilityMessage.cxx
  igtlConditionVariable.cxx
  igtlFastMutexLock.cxx
  igtlImageMessage.cxx
  igtlImageMessage2.cxx
  igtlLightObject.cxx
  igtlMath.cxx
  igtlMessageBase.cxx
  igtlMessageFactory.cxx
  igtlMultiThreader.cxx
  igtlMutexLock.cxx
  igtlOSUtil.cxx
  igtlObject.cxx
  igtlObjectFactoryBase.cxx
  igtlPositionMessage.cxx
  igtlServerSocket.cxx
  igtlSessionManager.cxx
  igtlSimpleFastMutexLock.cxx
  igtlSocket.cxx
  igtlStatusMessage.cxx
  igtlTimeStamp.cxx
  igtlTransformMessage.cxx
  )

SET(OpenIGTLink_INCLUDE_FILES)
IF( MSVC OR ${CMAKE_GENERATOR} MATCHES "Xcode" )
  LIST(APPEND OpenIGTLink_INCLUDE_FILES
    igtlutil/igtl_header.h
    igtlutil/igtl_image.h
    igtlutil/igtl_position.h
    igtlutil/igtl_transform.h
    igtlutil/igtl_types.h
    igtlutil/igtl_util.h
    igtlutil/igtl_capability.h
    igtlutil/igtl_win32header.h
    igtlMessageHandler.h
    igtlMessageHandlerMacro.h
    igtlCapabilityMessage.h
    igtlClientSocket.h
    igtlConditionVariable.h
    igtlCreateObjectFunction.h
    igtlFastMutexLock.h
    igtlImageMessage.h
    igtlImageMessage2.h
    igtlLightObject.h
    igtlMacro.h
    igtlMath.h
    igtlMessageBase.h
    igtlMessageFactory.h
    igtlMessageHeader.h
    igtlMultiThreader.h
    igtlMutexLock.h
    igtlObjectFactory.h
    igtlOSUtil.h
    igtlObject.h
    igtlObjectFactoryBase.h
    igtlPositionMessage.h
    igtlServerSocket.h
    igtlSessionManager.h
    igtlSimpleFastMutexLock.h
    igtlSmartPointer.h
    igtlSocket.h
    igtlStatusMessage.h
    igtlTimeStamp.h
    igtlTransformMessage.h
    igtlTypes.h
    igtlWin32Header.h
    igtlWindows.h
    igtlCommon.h
  )
ENDIF()

# Add support for OpenIGTLink version 2
IF (${OpenIGTLink_PROTOCOL_VERSION} GREATER "1" )
  LIST(APPEND OpenIGTLink_SOURCES
    igtlutil/igtl_colortable.c
    igtlutil/igtl_imgmeta.c
    igtlutil/igtl_lbmeta.c
    igtlutil/igtl_point.c
    igtlutil/igtl_tdata.c
    igtlutil/igtl_qtdata.c
    igtlutil/igtl_trajectory.c
    igtlutil/igtl_unit.c
    igtlutil/igtl_sensor.c
    igtlutil/igtl_string.c
    igtlutil/igtl_ndarray.c
    igtlutil/igtl_bind.c
    igtlutil/igtl_qtrans.c
    igtlutil/igtl_polydata.c
    igtlColorTableMessage.cxx
    igtlImageMetaMessage.cxx
    igtlLabelMetaMessage.cxx
    igtlPointMessage.cxx
    igtlTrackingDataMessage.cxx
    igtlPolyDataMessage.cxx
    igtlQuaternionTrackingDataMessage.cxx
    igtlTrajectoryMessage.cxx
    igtlStringMessage.cxx
    igtlUnit.cxx
    igtlSensorMessage.cxx
    igtlBindMessage.cxx
    igtlNDArrayMessage.cxx
    )
  IF( MSVC OR ${CMAKE_GENERATOR} MATCHES "Xcode" )
    LIST(APPEND OpenIGTLink_INCLUDE_FILES
      igtlutil/igtl_colortable.h
      igtlutil/igtl_imgmeta.h
      igtlutil/igtl_lbmeta.h
      igtlutil/igtl_point.h
      igtlutil/igtl_tdata.h
      igtlutil/igtl_qtdata.h
      igtlutil/igtl_trajectory.h
      igtlutil/igtl_unit.h
      igtlutil/igtl_sensor.h
      igtlutil/igtl_string.h
      igtlutil/igtl_ndarray.h
      igtlutil/igtl_bind.h
      igtlutil/igtl_qtrans.h
      igtlutil/igtl_polydata.h
      igtlColorTableMessage.h
      igtlImageMetaMessage.h
      igtlLabelMetaMessage.h
      igtlPointMessage.h
      igtlTrackingDataMessage.h
      igtlPolyDataMessage.h
      igtlQuaternionTrackingDataMessage.h
      igtlTrajectoryMessage.h
      igtlStringMessage.h
      igtlUnit.h
      igtlSensorMessage.h
      igtlBindMessage.h
      igtlNDArrayMessage.h
      )
  ENDIF()
ENDIF()

# Add support for OpenIGTLink version 3
IF(${OpenIGTLink_PROTOCOL_VERSION} GREATER "2" )
  LIST(APPEND OpenIGTLink_SOURCES
    igtlCommandMessage.cxx
    igtlQueryMessage.cxx
    igtlutil/igtl_command.c
    igtlutil/igtl_query.c
    )
  IF( MSVC OR ${CMAKE_GENERATOR} MATCHES "Xcode" )
    LIST(APPEND OpenIGTLink_INCLUDE_FILES
      igtlCommandMessage.h
      igtlQueryMessage.h
      igtlutil/igtl_command.h
      igtlutil/igtl_query.h
      )
  ENDIF()
  
  	set (WEBSOCKETPP_PLATFORM_LIBS "")
	set (WEBSOCKETPP_PLATFORM_TLS_LIBS "")
	set (WEBSOCKETPP_BOOST_LIBS "")

	# VC9 and C++11 reasoning
	if (ENABLE_CPP11 AND MSVC AND MSVC90)
		message("* Detected Visual Studio 9 2008, disabling C++11 support.")
		set (ENABLE_CPP11 FALSE)
	endif ()

	# Detect clang. Not officially reported by cmake.
	execute_process(COMMAND "${CMAKE_CXX_COMPILER}" "-v" ERROR_VARIABLE CXX_VER_STDERR)
	if ("${CXX_VER_STDERR}" MATCHES ".*clang.*")
		set (CMAKE_COMPILER_IS_CLANGXX 1)
	endif ()

	# C++11 defines
	if (ENABLE_CPP11)
		if (MSVC)
			add_definitions (-D_WEBSOCKETPP_CPP11_FUNCTIONAL_)
			add_definitions (-D_WEBSOCKETPP_CPP11_SYSTEM_ERROR_)
			add_definitions (-D_WEBSOCKETPP_CPP11_RANDOM_DEVICE_)
			add_definitions (-D_WEBSOCKETPP_CPP11_MEMORY_)
		else()
			add_definitions (-D_WEBSOCKETPP_CPP11_STL_)
		endif()
	endif ()

	# Visual studio
	if (MSVC)
		set (WEBSOCKETPP_BOOST_LIBS system thread)
		set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /GL /Gy /GF /Ox /Ob2 /Ot /Oi /MP /arch:SSE2 /fp:fast")
		set (CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /LTCG /INCREMENTAL:NO /OPT:REF /OPT:ICF")
		add_definitions (/W3 /wd4996 /wd4995 /wd4355)
		add_definitions (-DUNICODE -D_UNICODE)
		add_definitions (-D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS)
		add_definitions (-DNOMINMAX)
	endif ()

	# g++
	if (CMAKE_COMPILER_IS_GNUCXX)
		set (WEBSOCKETPP_PLATFORM_LIBS pthread rt)
		set (WEBSOCKETPP_PLATFORM_TLS_LIBS ssl crypto)
		set (WEBSOCKETPP_BOOST_LIBS system thread)
		set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
		if (NOT APPLE)
			add_definitions (-DNDEBUG -Wall -Wcast-align) # todo: should we use CMAKE_C_FLAGS for these?
		endif ()

		# Try to detect version. Note: Not tested!
		execute_process (COMMAND ${CMAKE_CXX_COMPILER} "-dumpversion" OUTPUT_VARIABLE GCC_VERSION)
		if ("${GCC_VERSION}" STRGREATER "4.4.0")
			message("* C++11 support partially enabled due to GCC version ${GCC_VERSION}")
			set (WEBSOCKETPP_BOOST_LIBS system thread)
		endif ()
	endif ()

	# clang
	if (CMAKE_COMPILER_IS_CLANGXX)
		if (NOT APPLE)
			set (WEBSOCKETPP_PLATFORM_LIBS pthread rt)
		else()
			set (WEBSOCKETPP_PLATFORM_LIBS pthread)
		endif()
		set (WEBSOCKETPP_PLATFORM_TLS_LIBS ssl crypto)
		set (WEBSOCKETPP_BOOST_LIBS system thread)
		set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -stdlib=libc++") # todo: is libc++ really needed here?
		if (NOT APPLE)
			add_definitions (-DNDEBUG -Wall -Wno-padded) # todo: should we use CMAKE_C_FLAGS for these?
		endif ()
	endif ()

	# OSX, can override above.
	if (APPLE)
		add_definitions (-DNDEBUG -Wall)
	endif ()

	if (BUILD_EXAMPLES)
		list (APPEND WEBSOCKETPP_BOOST_LIBS random)
	endif()

	if (BUILD_TESTS)
		list (APPEND WEBSOCKETPP_BOOST_LIBS unit_test_framework)
	endif()

	############ Dependencies

	# Set BOOST_ROOT env variable or pass with cmake -DBOOST_ROOT=path.
	# BOOST_ROOT can also be defined by a previous run from cmake cache.
	if (NOT "$ENV{BOOST_ROOT_CPP11}" STREQUAL "")
		# Scons documentation for BOOST_ROOT_CPP11:
		# "look for optional second boostroot compiled with clang's libc++ STL library
		# this prevents warnings/errors when linking code built with two different
		# incompatible STL libraries."
		file (TO_CMAKE_PATH "$ENV{BOOST_ROOT_CPP11}" BOOST_ROOT)
		set (BOOST_ROOT ${BOOST_ROOT} CACHE PATH "BOOST_ROOT dependency path" FORCE)
	endif ()
	if ("${BOOST_ROOT}" STREQUAL "")
		file (TO_CMAKE_PATH "$ENV{BOOST_ROOT}" BOOST_ROOT)
		# Cache BOOST_ROOT for runs that do not define $ENV{BOOST_ROOT}.
		set (BOOST_ROOT ${BOOST_ROOT} CACHE PATH "BOOST_ROOT dependency path" FORCE)
	endif ()

	message ("* Configuring Boost")
	message (STATUS "-- Using BOOST_ROOT")
	message (STATUS "       " ${BOOST_ROOT})

	if (MSVC)
		set (Boost_USE_MULTITHREADED TRUE)
		set (Boost_USE_STATIC_LIBS TRUE)
	else ()
		set (Boost_USE_MULTITHREADED FALSE)
		set (Boost_USE_STATIC_LIBS FALSE)
	endif ()

	if (BOOST_STATIC)
		set (Boost_USE_STATIC_LIBS TRUE)
	endif ()

	if (NOT Boost_USE_STATIC_LIBS)
		add_definitions (/DBOOST_TEST_DYN_LINK)
	endif ()

	set (Boost_FIND_REQUIRED TRUE)
	set (Boost_FIND_QUIETLY TRUE)
	set (Boost_DEBUG FALSE)
	set (Boost_USE_MULTITHREADED TRUE)
	set (Boost_ADDITIONAL_VERSIONS "1.39.0" "1.40.0" "1.41.0" "1.42.0" "1.43.0" "1.44.0" "1.46.1") # todo: someone who knows better spesify these!

	find_package (Boost 1.39.0 COMPONENTS "${WEBSOCKETPP_BOOST_LIBS}")

	if (Boost_FOUND)
		# Boost is a project wide global dependency.
		include_directories (${Boost_INCLUDE_DIRS})
		link_directories (${Boost_LIBRARY_DIRS})

		# Pretty print status
		message (STATUS "-- Include Directories")
		foreach (include_dir ${Boost_INCLUDE_DIRS})
			message (STATUS "       " ${include_dir})
		endforeach ()
		message (STATUS "-- Library Directories")
		foreach (library_dir ${Boost_LIBRARY_DIRS})
			message (STATUS "       " ${library_dir})
		endforeach ()
		message (STATUS "-- Libraries")
		foreach (boost_lib ${Boost_LIBRARIES})
			message (STATUS "       " ${boost_lib})
		endforeach ()
		message ("")
	else ()
		message (FATAL_ERROR "Failed to find required dependency: boost")
	endif ()

	find_package(OpenSSL)
	find_package(ZLIB)
  add_executable (telemetry_server2 telemetry_server2.cpp)

  include_directories (${PROJECT_BINARY_DIR}/websocketpp ${PROJECT_BINARY_DIR}/websocketpp/websocketpp)

  target_link_libraries(telemetry_server2 ${WEBSOCKETPP_PLATFORM_LIBS} ${Boost_LIBRARIES})


ENDIF()

ADD_LIBRARY(OpenIGTLink ${OpenIGTLink_SOURCES} ${OpenIGTLink_INCLUDE_FILES})
target_include_directories(OpenIGTLink ${OpenIGTLink_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(OpenIGTLink PUBLIC ${LINK_LIBS} ${Boost_LIBRARIES})

IF(MSVC)
  target_compile_options(OpenIGTLink PRIVATE /MP)
ENDIF()

SET_TARGET_PROPERTIES(OpenIGTLink PROPERTIES
  VERSION ${OpenIGTLink_VERSION_MAJOR}.${OpenIGTLink_VERSION_MINOR}.${OpenIGTLink_VERSION_PATCH}
  SOVERSION ${OpenIGTLink_VERSION_MAJOR}
  )

INSTALL(FILES ${OpenIGTLink_INCLUDE_FILES}
  DESTINATION ${OpenIGTLink_INSTALL_INCLUDE_DIR}
  COMPONENT Development)

INSTALL(TARGETS OpenIGTLink
  RUNTIME DESTINATION ${OpenIGTLink_INSTALL_BIN_DIR} COMPONENT RuntimeLibraries
  LIBRARY DESTINATION ${OpenIGTLink_INSTALL_LIB_DIR} COMPONENT RuntimeLibraries
  ARCHIVE DESTINATION ${OpenIGTLink_INSTALL_LIB_DIR} COMPONENT Development)