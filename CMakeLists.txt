cmake_minimum_required(VERSION 3.2)
project(foolsock)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(SOURCE_FILES smthrift.c php_thrift_protocol.cpp php_smthrift.h)

INCLUDE_DIRECTORIES(
    .
    /usr/local/Cellar/php71/7.1.5_17/include/php/
    /usr/local/Cellar/php71/7.1.5_17/include/php/main
    /usr/local/Cellar/php71/7.1.5_17/include/php/Zend
)
add_executable(foolsock ${SOURCE_FILES})