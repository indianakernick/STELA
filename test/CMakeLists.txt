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

SETUP_TEST(Lexer "src/lexer.cpp")
SETUP_TEST(Syntax "src/syntax.cpp")
SETUP_TEST(Semantics "src/semantics.cpp")
SETUP_TEST(Generation "src/generation.cpp")
SETUP_TEST(Format "src/format.cpp")
