cmake_minimum_required(VERSION 2.6)
project(tgui2 CXX)

# Set USER_INCLUDE_PATH as may be necessary
# e.g., -DUSER_INCLUDE_PATH=/home/trent/local/include

if(APPLE)
	SET(CMAKE_CXX_COMPILER "g++")
	set(CMAKE_CXX_FLAGS "-stdlib=libstdc++")
endif()

if(NOT ("${USER_INCLUDE_PATH}" STREQUAL ""))
	include_directories(${USER_INCLUDE_PATH})
endif(NOT ("${USER_INCLUDE_PATH}" STREQUAL ""))

set(SOURCES tgui2.cpp tgui2_widgets.cpp)

add_library(tgui2 STATIC ${SOURCES})
set_target_properties(tgui2
	PROPERTIES
	OUTPUT_NAME tgui2
	)
