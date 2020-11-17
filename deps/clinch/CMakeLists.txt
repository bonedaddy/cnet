# cmake file for use with clib

add_library(libclinch
    SHARED
        ./deps/clinch/command_line.h
        ./deps/clinch/command_line.c
)

target_compile_options(libclinch PRIVATE ${flags})
target_link_libraries(libclinch libargtable3 libulog)