cmake_minimum_required(VERSION 3.2)

add_executable(Test
        "src/lexer.cpp"
        "src/lexer.hpp"
        "src/log counter.cpp"
        "src/log counter.hpp"
        "src/macros.hpp"
        "src/main.cpp"
        "src/syntax.cpp"
        "src/syntax.hpp"
        "src/semantics.cpp"
        "src/semantics.hpp"
        "src/format.cpp"
        "src/format.hpp"
        "src/generation.cpp"
        "src/generation.hpp"
)

set_target_properties(Test
        PROPERTIES OUTPUT_NAME "suite"
)

if(TEST_COVERAGE)
        set_target_properties(Test
                PROPERTIES
                COMPILE_FLAGS "-DNDEBUG -DTEST_COVERAGE -O0 --coverage -fno-inline -fno-inline-small-functions -fno-default-inline"
                LINK_FLAGS "--coverage -lgcov"
        )
endif()

target_link_libraries(Test
        PRIVATE
        STELA
)
