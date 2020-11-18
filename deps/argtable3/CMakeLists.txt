# configured for use with clib

file(GLOB_RECURSE ARGTABLE3_SOURCES
    ./deps/argtable3/*.h
    ./deps/argtable3/*.c
)

add_library(libargtable3
    SHARED
        ${ARGTABLE3_SOURCES}
)

target_compile_options(libargtable3 PRIVATE ${flags})
target_link_libraries(libargtable3 m)