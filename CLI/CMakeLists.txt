cmake_minimum_required(VERSION 3.2)

add_executable(cli
        "src/main.cpp"
)

set_target_properties(cli
        PROPERTIES OUTPUT_NAME "stela"
)

target_link_libraries(cli
        PRIVATE
        STELA
)

target_compile_options(cli
        PRIVATE
        -Wall -Wextra -Weffc++ -Wpedantic -Wno-documentation
)

install(TARGETS cli
        RUNTIME DESTINATION bin
)
