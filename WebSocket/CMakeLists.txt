cmake_minimum_required(VERSION 2.8)
project(WebSocket NONE)

# Download and unpack Websocket at configure time
configure_file(CMakeListsWebSocket.txt.in
								 download/CMakeLists.txt)

execute_process(COMMAND "${CMAKE_COMMAND}" -G"${CMAKE_GENERATOR}" . 
		WORKING_DIRECTORY "${PROJECT_BINARY_DIR}/download" )								 
execute_process(COMMAND "${CMAKE_COMMAND}" --build . 
		WORKING_DIRECTORY "${PROJECT_BINARY_DIR}/download" )		
		
		