cmake_minimum_required(VERSION 3.20.0)
# Add custom board root
set(BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(medical_wearable_app)

target_sources(app PRIVATE 
    src/main.c
    src/system.c
    src/thread_manager.c
    src/safe_queue.c
    src/safe_buffer.c
    src/medical_device.c
    src/diagnostics.c
    src/config.c
    src/hardware.c
    src/shell_commands.c
)

# Register shell commands
zephyr_library_sources_ifdef(CONFIG_SHELL
    src/shell_commands.c
)

