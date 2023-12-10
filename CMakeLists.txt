cmake_minimum_required(VERSION 3.21)
project(mmkv)

set(CMAKE_CXX_STANDARD 17)
find_package(Boost REQUIRED COMPONENTS serialization)
if(NOT Boost_FOUND)
    message("Not FOUND BOOST")
endif ()

add_executable(mmkv main.cpp
        mmkv.hpp
        file_struct.h)
target_link_libraries(mmkv ${Boost_LIBRARIES})