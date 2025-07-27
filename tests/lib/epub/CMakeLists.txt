cmake_minimum_required(VERSION 3.20.0)

project(app)
find_package(Zephyr COMPONENTS unittest REQUIRED HINTS $ENV{ZEPHYR_BASE})
target_sources(testbinary
    PRIVATE
    main.c 
    ${ZEPHYR_BASE}/lib/epub/epub.c
)