cmake_minimum_required(VERSION 3.10)
project(MyProject VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
    main.cc
    src/irbuilder.cc
    src/liveness.cc
    src/regalloc.cc
    src/inlining.cc
)

add_executable(build_ir ${SOURCES})

target_include_directories(build_ir PRIVATE include)
target_include_directories(build_ir PRIVATE src)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(build_ir PRIVATE -Wall -Wextra -Wpedantic)
elseif(MSVC)
    target_compile_options(build_ir PRIVATE /W4)
endif()

add_library(ir_lib STATIC src/irbuilder.cc src/liveness.cc src/regalloc.cc src/inlining.cc)
target_include_directories(ir_lib PUBLIC include src)

target_link_libraries(build_ir PRIVATE ir_lib)

enable_testing()

find_package(GTest REQUIRED)

add_executable(test_dominators tests/test_dominators_set.cc 
    tests/test_loop.cc tests/loop.cc tests/test_opt.cc tests/test_liveness.cc tests/test_regalloc.cc tests/test_inlining.cc tests/test_checks.cc)
# add_executable(test_dominators tests/test_dominators_set.cc tests/loop.cc)
target_include_directories(test_dominators PRIVATE tests)

target_link_libraries(test_dominators PRIVATE ir_lib GTest::gtest GTest::gtest_main)

add_test(NAME GeneralTest COMMAND test_dominators)
add_test(NAME Test1 COMMAND test_dominators)
add_test(NAME Test2 COMMAND test_dominators)
add_test(NAME Test3 COMMAND test_dominators)

add_test(NAME LoopAnalysis COMMAND test_dominators)
add_test(NAME OptimizationTest COMMAND test_dominators)
add_test(NAME LivenessTest COMMAND test_dominators)
add_test(NAME RegAllocTest COMMAND test_dominators)

add_test(NAME InliningTest COMMAND test_dominators)
add_test(NAME CheckOptimization COMMAND test_dominators)
