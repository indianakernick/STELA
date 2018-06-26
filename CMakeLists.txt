cmake_minimum_required(VERSION 3.10)
project(STELA)

add_library(STELA STATIC
        "src/lexical analysis.cpp"
        "src/log.cpp"
        "src/log output.cpp"
        "src/log output.hpp"
        "src/number literal.cpp"
        "src/number literal.hpp"
        "src/parse tokens.cpp"
        "src/parse tokens.hpp"
        "src/syntax analysis.cpp"
)

file(GLOB HEADERS_LIST "${CMAKE_CURRENT_SOURCE_DIR}/include/STELA/*.hpp")
set_target_properties(STELA PROPERTIES PUBLIC_HEADER "${HEADERS_LIST}")

target_include_directories(STELA
        INTERFACE
        include
        PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/STELA>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        /usr/local/include
)

target_compile_options(STELA
        PRIVATE
        -Wall -Wextra -Weffc++ -Wpedantic -Wno-documentation
)

target_compile_features(STELA
        PRIVATE
        cxx_std_17
)

install(TARGETS STELA
        ARCHIVE DESTINATION lib
        PUBLIC_HEADER DESTINATION include/STELA
)

add_subdirectory(test)
add_subdirectory(CLI)