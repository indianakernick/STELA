cmake_minimum_required(VERSION 3.10)

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

target_compile_features(cli
        PRIVATE
        cxx_std_17
)

install(TARGETS cli
        RUNTIME DESTINATION bin
)