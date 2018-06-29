cmake_minimum_required(VERSION 3.11)
project(STELA)

include(FetchContent)

add_library(STELA STATIC
        "include/STELA/ast.hpp"
        "include/STELA/lexical analysis.hpp"
        "include/STELA/location.hpp"
        "include/STELA/log.hpp"
        "include/STELA/syntax analysis.hpp"
        "include/STELA/token.hpp"
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

FetchContent_Declare(Simpleton
        GIT_REPOSITORY https://github.com/Kerndog73/Simpleton-Engine.git
)

FetchContent_GetProperties(Simpleton)
if(NOT Simpleton_POPULATED)
        FetchContent_Populate(Simpleton)
endif()

target_include_directories(STELA
        INTERFACE
        include
        PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/STELA>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        "${FETCHCONTENT_BASE_DIR}/simpleton-src"
)

target_compile_options(STELA
        PRIVATE
        -Wall -Wextra -Weffc++ -Wpedantic -Wno-documentation
)

target_compile_features(STELA
        PUBLIC
        cxx_std_17
)

install(TARGETS STELA
        ARCHIVE DESTINATION lib
        PUBLIC_HEADER DESTINATION include/STELA
)

add_subdirectory(test)
add_subdirectory(CLI)
