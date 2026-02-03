cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#

# ============================================================
# Toolchain / standards
# ============================================================
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# ============================================================
# Project
# ============================================================
set(CMAKE_PROJECT_NAME CNC_PCB_FIRMWARE)
project(${CMAKE_PROJECT_NAME})

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Enable languages
enable_language(C CXX ASM)

# Generate compile_commands.json (clangd, VSCode)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ============================================================
# Target
# ============================================================
add_executable(${CMAKE_PROJECT_NAME})

# STM32CubeMX generated code
add_subdirectory(cmake/stm32cubemx)

# ============================================================
# Sources (Twoje C++ pliki)
# ============================================================
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    Core/Src/cpp_main.cpp

    Core/Src/hardware/ili9341.cpp
    Core/Src/Gcode/GcodeParser.cpp
    Core/Src/common/systemError.cpp
    Core/Src/hardware/tmc2209.cpp
     Core/Src/hardware/xpt2046.cpp
)

# ============================================================
# Include paths
# ============================================================
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    Core/Inc
    Core/Inc/common
    Core/Inc/hardware
    Core/Inc/display
    Core/Inc/planner
    Core/Inc/Gcode

    Middlewares/Third_Party/FreeRTOS/Source/include
    Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2
    Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F
)

# ============================================================
# Compile definitions (opcjonalnie)
# ============================================================
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    USE_HAL_DRIVER
    STM32F405xx
)

# ============================================================
# C++ flags (embedded-friendly)
# ============================================================
target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:
        -fno-rtti
        -fno-exceptions
        -fno-threadsafe-statics
        -Wno-register
    >
)

# ============================================================
# Linker options
# ============================================================
target_link_options(${CMAKE_PROJECT_NAME} PRIVATE
    -u _printf_float
    -Wl,--print-memory-usage
)

# CubeMX bugfix (libob.a)
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# ============================================================
# Libraries
# ============================================================
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx
    m
)
