cmake_minimum_required(VERSION 3.10)
project(MyProject VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
    main.cc
    src/irbuilder.cc
)

add_executable(build_ir ${SOURCES})

target_include_directories(build_ir PRIVATE include)
target_include_directories(build_ir PRIVATE src)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(build_ir PRIVATE -Wall -Wextra -Wpedantic)
elseif(MSVC)
    target_compile_options(build_ir PRIVATE /W4)
endif()