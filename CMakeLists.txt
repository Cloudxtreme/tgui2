cmake_minimum_required(VERSION 2.6)
project(tgui2 CXX)

# Set USER_INCLUDE_PATH as may be necessary
# e.g., -DUSER_INCLUDE_PATH=/home/trent/local/include

if(NOT ("${USER_INCLUDE_PATH}" STREQUAL ""))
	include_directories(${USER_INCLUDE_PATH})
endif(NOT ("${USER_INCLUDE_PATH}" STREQUAL ""))

if(WIN32)
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_RELEASE} /MTd")
endif()

set(SOURCES tgui2.cpp tgui2_widgets.cpp)

if(CMAKE_BUILD_TYPE MATCHES Debug)
	set(LIBNAME "tgui2d")
else()
	set(LIBNAME "tgui2")
endif()

add_library(${LIBNAME} STATIC ${SOURCES})

target_link_libraries(${LIBNAME} "allegro_monolith")