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
)

set_target_properties(Test
        PROPERTIES OUTPUT_NAME "suite"
)

target_link_libraries(Test
        PRIVATE
        STELA
)

target_compile_options(Test
        PRIVATE
        -Wall -Wextra -Weffc++ -Wpedantic -Wno-documentation
)
