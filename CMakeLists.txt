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
        "src/expr lookup.cpp"
        "src/expr lookup.hpp"
        "src/expr stack.cpp"
        "src/expr stack.hpp"
        "src/plain format.cpp"
        "src/symbol desc.cpp"
        "src/symbol desc.hpp"
        "src/check scopes.cpp"
        "src/check scopes.hpp"
        "src/code generation.cpp"
        "src/unreachable.hpp"
        "src/builtin code.cpp"
        "src/builtin code.hpp"
        "src/generate decl.cpp"
        "src/generate decl.hpp"
        "src/gen context.hpp"
        "src/gen string.hpp"
        "src/type instantiations.cpp"
        "src/type instantiations.hpp"
        "src/generate ids.cpp"
        "src/generate ids.hpp"
        "src/generate expr.cpp"
        "src/generate expr.hpp"
        "src/generate zero expr.cpp"
        "src/generate zero expr.hpp"
        "src/generate type.cpp"
        "src/generate type.hpp"
        "src/generate func.cpp"
        "src/generate func.hpp"
        "src/assert down cast.hpp"
        "src/c standard library.cpp"
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
        ${LLVM_INCLUDE_DIRS}
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/STELA>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${SOURCE_DIR}
)

target_compile_definitions(STELA
        PUBLIC
        ${LLVM_DEFINITIONS}
)

llvm_map_components_to_libnames(llvm_libs core native mcjit asmprinter asmparser)

message(STATUS "Include dirs: ${LLVM_INCLUDE_DIRS}")
message(STATUS "Definitions: ${LLVM_DEFINITIONS}")
message(STATUS "Libs: ${llvm_libs}")

# succeeds when linking with none of these
# fi  fails individually
# si  succeeds individually

# fi LLVMCore;
# fi LLVMMCJIT;
#    LLVMAsmPrinter;
#    LLVMAsmParser;
#    LLVMX86CodeGen;
#    LLVMX86AsmParser;
#    LLVMX86AsmPrinter;
#    LLVMX86Desc;
#    LLVMX86Disassembler;
#    LLVMX86Info;
#    LLVMX86Utils

target_link_libraries(STELA
        PUBLIC
        LLVMAsmPrinter
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

add_subdirectory(test)
add_subdirectory(CLI)
