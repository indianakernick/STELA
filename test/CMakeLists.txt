cmake_minimum_required(VERSION 3.2)

include(ExternalProject)

macro(SETUP_TEST NAME SOURCE)
    add_executable(${NAME} "${SOURCE}")
    target_link_libraries(${NAME}
        PRIVATE
        STELA
        gtest
        gtest_main
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
set(BENCHMARK_ENABLE_TESTING NO CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS NO CACHE BOOL "" FORCE)
set(CMAKE_BUILD_TYPE_SAVED "${CMAKE_BUILD_TYPE}")
set(CMAKE_BUILD_TYPE Release)
add_subdirectory(${GBENCH_DIR}/src ${GBENCH_DIR}/build)
set(CMAKE_BUILD_TYPE "${CMAKE_BUILD_TYPE_SAVED}")

SETUP_TEST(Lexer "src/lexer.cpp")
SETUP_TEST(Syntax "src/syntax.cpp")
SETUP_TEST(Semantics "src/semantics.cpp")
SETUP_TEST(Generation "src/generation.cpp")
SETUP_TEST(Format "src/format.cpp")

SETUP_BENCH(Compiler "src/compiler benchmark.cpp")
SETUP_BENCH(Language "src/language benchmark.cpp")
