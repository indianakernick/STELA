cmake_minimum_required(VERSION 3.9)
# need 3.8 for cxx_std_17
# need 3.9 for IPO
# I commented cxx_std_17 and tested successfully with cmake 3.2
project(STELA)

find_package(Git REQUIRED)
include(ExternalProject)

add_library(STELA STATIC
        "include/STELA/ast.hpp"
        "include/STELA/lexical analysis.hpp"
        "include/STELA/location.hpp"
        "include/STELA/log.hpp"
        "include/STELA/syntax analysis.hpp"
        "include/STELA/token.hpp"
        "include/STELA/symbols.hpp"
        "include/STELA/semantic analysis.hpp"
        "include/STELA/format.hpp"
        "include/STELA/console format.hpp"
        "include/STELA/html format.hpp"
        "include/STELA/modules.hpp"
        "src/lexical analysis.cpp"
        "src/log.cpp"
        "src/log output.cpp"
        "src/log output.hpp"
        "src/number literal.cpp"
        "src/number literal.hpp"
        "src/parse tokens.cpp"
        "src/parse tokens.hpp"
        "src/syntax analysis.cpp"
        "src/context stack.cpp"
        "src/context stack.hpp"
        "src/parse type.cpp"
        "src/parse type.hpp"
        "src/parse func.cpp"
        "src/parse func.hpp"
        "src/parse stat.cpp"
        "src/parse stat.hpp"
        "src/parse decl.cpp"
        "src/parse decl.hpp"
        "src/parse litr.cpp"
        "src/parse litr.hpp"
        "src/parse expr.cpp"
        "src/parse expr.hpp"
        "src/parse asgn.cpp"
        "src/parse asgn.hpp"
        "src/semantic analysis.cpp"
        "src/traverse.cpp"
        "src/traverse.hpp"
        "src/ast.cpp"
        "src/symbols.cpp"
        "src/format.cpp"
        "src/console format.cpp"
        "src/html format.cpp"
        "src/scope manager.cpp"
        "src/scope manager.hpp"
        "src/infer type.cpp"
        "src/infer type.hpp"
        "src/builtin symbols.cpp"
        "src/builtin symbols.hpp"
        "src/operator name.cpp"
        "src/operator name.hpp"
        "src/compare params args.cpp"
        "src/compare params args.hpp"
        "src/scope insert.cpp"
        "src/scope insert.hpp"
        "src/scope lookup.cpp"
        "src/scope lookup.hpp"
        "src/scope traverse.hpp"
        "src/compare types.cpp"
        "src/compare types.hpp"
        "src/modules.cpp"
        "src/intrusive ptr.hpp"
)

file(GLOB HEADERS_LIST "${CMAKE_CURRENT_SOURCE_DIR}/include/STELA/*.hpp")
set_target_properties(STELA PROPERTIES PUBLIC_HEADER "${HEADERS_LIST}")

ExternalProject_Add(
        Simpleton
        GIT_REPOSITORY https://github.com/Kerndog73/Simpleton-Engine.git
        UPDATE_COMMAND ${GIT_EXECUTABLE} pull origin master
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
)
ExternalProject_Get_Property(Simpleton SOURCE_DIR)

target_include_directories(STELA
        INTERFACE
        include
        PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/STELA>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${SOURCE_DIR}
)

target_compile_features(STELA
        PUBLIC
        cxx_std_17
)

include(CheckIPOSupported)

check_ipo_supported(RESULT result)
if(result)
        set_target_properties(STELA
                PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE
        )
endif()

install(TARGETS STELA
        ARCHIVE DESTINATION lib
        PUBLIC_HEADER DESTINATION include/STELA
)

add_subdirectory(test)
add_subdirectory(CLI)
