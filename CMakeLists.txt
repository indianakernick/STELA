cmake_minimum_required(VERSION 3.9)
# need 3.8 for cxx_std_17
# need 3.9 for IPO
project(STELA)

find_package(Git REQUIRED)
find_package(LLVM 7.0.0 REQUIRED CONFIG)
message(STATUS "Found LLVM: ${LLVM_DIR} ${LLVM_PACKAGE_VERSION}")
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
    "include/STELA/retain ptr.hpp"
    "include/STELA/plain format.hpp"
    "include/STELA/code generation.hpp"
    "include/STELA/c standard library.hpp"
    "include/STELA/llvm.hpp"
    "include/STELA/number.hpp"
    "include/STELA/binding.hpp"
    "src/Utils/unreachable.hpp"
    "src/Utils/assert down cast.hpp"
    "src/Utils/iterator range.hpp"
    "src/Utils/algorithms.hpp"
    "src/Utils/console color.hpp"
    "src/Utils/parse string.inl"
    "src/Utils/parse string.hpp"
    "src/Format/format.cpp"
    "src/Format/console format.cpp"
    "src/Format/html format.cpp"
    "src/Format/plain format.cpp"
    "src/CodeGen/inst data.hpp"
    "src/CodeGen/func instantiations.cpp"
    "src/CodeGen/func instantiations.hpp"
    "src/CodeGen/gen helpers.cpp"
    "src/CodeGen/gen helpers.hpp"
    "src/CodeGen/generate func.cpp"
    "src/CodeGen/generate array.cpp"
    "src/CodeGen/generate struct.cpp"
    "src/CodeGen/generate pointer.cpp"
    "src/CodeGen/generate builtin.cpp"
    "src/CodeGen/generate closure.cpp"
    "src/CodeGen/generate closure.hpp"
    "src/CodeGen/categories.cpp"
    "src/CodeGen/categories.hpp"
    "src/CodeGen/lifetime exprs.cpp"
    "src/CodeGen/lifetime exprs.hpp"
    "src/CodeGen/compare exprs.cpp"
    "src/CodeGen/compare exprs.hpp"
    "src/CodeGen/code generation.cpp"
    "src/CodeGen/llvm.cpp"
    "src/CodeGen/gen context.hpp"
    "src/CodeGen/optimize module.cpp"
    "src/CodeGen/optimize module.hpp"
    "src/CodeGen/generate decl.cpp"
    "src/CodeGen/generate decl.hpp"
    "src/CodeGen/generate stat.cpp"
    "src/CodeGen/generate stat.hpp"
    "src/CodeGen/generate type.cpp"
    "src/CodeGen/generate type.hpp"
    "src/CodeGen/generate expr.cpp"
    "src/CodeGen/generate expr.hpp"
    "src/CodeGen/lower expressions.cpp"
    "src/CodeGen/lower expressions.hpp"
    "src/CodeGen/function builder.cpp"
    "src/CodeGen/function builder.hpp"
    "src/CodeGen/gen types.cpp"
    "src/CodeGen/gen types.hpp"
    "src/Semantic/semantic analysis.cpp"
    "src/Semantic/modules.cpp"
    "src/Semantic/ast.cpp"
    "src/Semantic/symbols.cpp"
    "src/Semantic/traverse.cpp"
    "src/Semantic/traverse.hpp"
    "src/Semantic/scope manager.cpp"
    "src/Semantic/scope manager.hpp"
    "src/Semantic/infer type.cpp"
    "src/Semantic/infer type.hpp"
    "src/Semantic/builtin symbols.cpp"
    "src/Semantic/builtin symbols.hpp"
    "src/Semantic/operator name.cpp"
    "src/Semantic/operator name.hpp"
    "src/Semantic/compare params args.cpp"
    "src/Semantic/compare params args.hpp"
    "src/Semantic/scope insert.cpp"
    "src/Semantic/scope insert.hpp"
    "src/Semantic/scope lookup.cpp"
    "src/Semantic/scope lookup.hpp"
    "src/Semantic/expr lookup.cpp"
    "src/Semantic/expr lookup.hpp"
    "src/Semantic/expr stack.cpp"
    "src/Semantic/expr stack.hpp"
    "src/Semantic/scope traverse.hpp"
    "src/Semantic/compare types.cpp"
    "src/Semantic/compare types.hpp"
    "src/Semantic/context.hpp"
    "src/Semantic/symbol desc.cpp"
    "src/Semantic/symbol desc.hpp"
    "src/Semantic/check scopes.cpp"
    "src/Semantic/check scopes.hpp"
    "src/Semantic/c standard library.cpp"
    "src/Semantic/check missing return.cpp"
    "src/Semantic/check missing return.hpp"
    "src/Syntax/syntax analysis.cpp"
    "src/Syntax/parse tokens.cpp"
    "src/Syntax/parse tokens.hpp"
    "src/Syntax/context stack.cpp"
    "src/Syntax/context stack.hpp"
    "src/Syntax/parse type.cpp"
    "src/Syntax/parse type.hpp"
    "src/Syntax/parse func.cpp"
    "src/Syntax/parse func.hpp"
    "src/Syntax/parse stat.cpp"
    "src/Syntax/parse stat.hpp"
    "src/Syntax/parse decl.cpp"
    "src/Syntax/parse decl.hpp"
    "src/Syntax/parse litr.cpp"
    "src/Syntax/parse litr.hpp"
    "src/Syntax/parse expr.cpp"
    "src/Syntax/parse expr.hpp"
    "src/Syntax/parse asgn.cpp"
    "src/Syntax/parse asgn.hpp"
    "src/Lex/lexical analysis.cpp"
    "src/Lex/number literal.cpp"
    "src/Lex/number literal.hpp"
    "src/Log/log.cpp"
    "src/Log/log output.cpp"
    "src/Log/log output.hpp"
)

file(GLOB HEADERS_LIST "${CMAKE_CURRENT_SOURCE_DIR}/include/STELA/*.hpp")
set_target_properties(STELA PROPERTIES PUBLIC_HEADER "${HEADERS_LIST}")

target_include_directories(STELA
    INTERFACE
    include
    PUBLIC
    ${LLVM_INCLUDE_DIRS}
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/STELA>
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/deps/simpleton/include
)

target_compile_definitions(STELA
    PUBLIC
    ${LLVM_DEFINITIONS}
)

llvm_map_components_to_libnames(llvm_libs core native mcjit asmprinter asmparser linker instrumentation vectorize ipo)

if(TEST_COVERAGE)
    set(_COLLECT_LTO_WRAPPER_TEXT "COLLECT_LTO_WRAPPER=")
    execute_process(
        COMMAND bash -c "$0 -v 2>&1 | grep ${_COLLECT_LTO_WRAPPER_TEXT} | sed s/${_COLLECT_LTO_WRAPPER_TEXT}//" "${CMAKE_CXX_COMPILER}"
        OUTPUT_VARIABLE _GAINARIE_COLLECT_TMP_VAR
    )
    get_filename_component(_GAINARIE_GCC_DEFAULT_PATH ${_GAINARIE_COLLECT_TMP_VAR} DIRECTORY)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L${_GAINARIE_GCC_DEFAULT_PATH}")
    #set (GCCOPT "${GCCOPT} -L${_GAINARIE_GCC_DEFAULT_PATH}")
endif()

target_link_libraries(STELA
    PUBLIC
    ${llvm_libs}
)

target_compile_features(STELA
    PUBLIC
    cxx_std_17
)

if(TEST_COVERAGE)
    set(CMAKE_CXX_FLAGS "-DNDEBUG -DTEST_COVERAGE -O0 --coverage -fno-inline -fno-inline-small-functions -fno-default-inline")
endif()

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

add_subdirectory(CLI)
enable_testing()
add_subdirectory(test)
