cmake_minimum_required(VERSION 3.20.0)

# Add custom board root
set(BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nordic_medical_platform)

target_sources(app PRIVATE 
    src/main.c
    src/hardware.c
)

