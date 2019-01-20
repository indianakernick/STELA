cmake_minimum_required(VERSION 3.2)

include(ExternalProject)

macro(SETUP_TEST NAME SOURCE)
    add_executable(${NAME} "${SOURCE}")
    target_link_libraries(${NAME}
        PRIVATE
        STELA
        GTest::Main 
        Threads::Threads
    )
    add_test(NAME ${NAME} COMMAND ${NAME})
endmacro()

macro(SETUP_BENCH NAME SOURCE)
    add_executable(${NAME} "${SOURCE}")
    target_link_libraries(${NAME}
        PRIVATE
        STELA
        benchmark
        benchmark_main
    )
    add_test(NAME ${NAME} COMMAND ${NAME})
endmacro()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(GTEST_DIR ${PROJECT_SOURCE_DIR}/deps/googletest)
configure_file(${PROJECT_SOURCE_DIR}/cmake/in/googletest.in ${GTEST_DIR}/CMakeLists.txt)
execute_process(
    COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" . 
    WORKING_DIRECTORY ${GTEST_DIR}
)
execute_process(
    COMMAND ${CMAKE_COMMAND} --build . 
    WORKING_DIRECTORY ${GTEST_DIR}
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
add_subdirectory(${GTEST_DIR}/src ${GTEST_DIR}/build)
add_library(GTest::Main ALIAS gtest_main)

set(GBENCH_DIR ${PROJECT_SOURCE_DIR}/deps/googlebenchmark)
configure_file(${PROJECT_SOURCE_DIR}/cmake/in/googlebenchmark.in ${GBENCH_DIR}/CMakeLists.txt)
execute_process(
    COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" . 
    WORKING_DIRECTORY ${GBENCH_DIR}
)
execute_process(
    COMMAND ${CMAKE_COMMAND} --build . 
    WORKING_DIRECTORY ${GBENCH_DIR}
)
set(CMAKE_BUILD_TYPE Release)
set(BENCHMARK_ENABLE_LTO YES)
set(BENCHMARK_ENABLE_TESTING NO)
add_subdirectory(${GBENCH_DIR}/src ${GBENCH_DIR}/build)

SETUP_TEST(Lexer "src/lexer.cpp")
SETUP_TEST(Syntax "src/syntax.cpp")
SETUP_TEST(Semantics "src/semantics.cpp")
SETUP_TEST(Generation "src/generation.cpp")
SETUP_TEST(Format "src/format.cpp")

SETUP_BENCH(Compiler "src/compiler benchmark.cpp")
SETUP_BENCH(Language "src/language benchmark.cpp")
