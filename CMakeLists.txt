cmake_minimum_required(VERSION 3.16)
project(VertipaqDictinary)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(STRING_ENCODING_TYPE "ICONV" CACHE STRING "Set the way strings have to be encoded (ICONV|WIN32API|NONE|...)")
add_definitions(-DKS_STR_ENCODING_ICONV)


include_directories(third_party/kaitai)
include_directories(third_party)
include_directories(include)

set(KAITAI_SOURCES
    third_party/kaitai/kaitaistream.cpp)

add_executable(VertipaqDictinary main.cpp column_data_dictionary.cpp ${KAITAI_SOURCES})
