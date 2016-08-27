project(boostasiotcp)
cmake_minimum_required(VERSION 2.8)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)

if(COMPILER_SUPPORTS_CXX11)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
        message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

set(BOOST_ROOT "/opt/boost_1_60_0")
find_package(Boost REQUIRED COMPONENTS filesystem system coroutine)

if(NOT Boost_FOUND)
  message(FATAL_ERROR "未找到Boost库，请安装或者指明所在目录")
endif()
include_directories(${Boost_INCLUDE_DIRS})

aux_source_directory(./server SRC_SERVER_LIST)
aux_source_directory(./client SRC_CLIENT_LIST)

add_executable(server ${SRC_SERVER_LIST})
target_link_libraries(server ${Boost_LIBRARIES})

add_executable(client ${SRC_CLIENT_LIST})
target_link_libraries(client ${Boost_LIBRARIES})


