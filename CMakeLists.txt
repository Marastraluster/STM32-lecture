cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME Program1)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Auto-collect user sources and headers
set(PROJECT_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src)
set(PROJECT_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc)
file(GLOB_RECURSE PROJECT_C_SOURCES CONFIGURE_DEPENDS
    "${PROJECT_SRC_DIR}/*.c"
)
# system_stm32f4xx.c is already provided by stm32cubemx/STM32_Drivers.
list(FILTER PROJECT_C_SOURCES EXCLUDE REGEX "/system_stm32f4xx\\.c$")
file(GLOB_RECURSE PROJECT_HEADERS CONFIGURE_DEPENDS
    "${PROJECT_INC_DIR}/*.h"
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    ${PROJECT_C_SOURCES}
    ${PROJECT_HEADERS}
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${PROJECT_INC_DIR}
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)

# Optional one-command DFU flash target.
find_program(STM32_PROGRAMMER_CLI
    NAMES STM32_Programmer_CLI STM32_Programmer_CLI.exe
)

if(STM32_PROGRAMMER_CLI)
    add_custom_target(dfu_flash
        COMMAND ${STM32_PROGRAMMER_CLI} -c port=USB1 -w $<TARGET_FILE:${CMAKE_PROJECT_NAME}> -v -rst
        DEPENDS ${CMAKE_PROJECT_NAME}
        COMMENT "Build and flash ${CMAKE_PROJECT_NAME} over USB-DFU"
    )
else()
    message(STATUS "STM32_Programmer_CLI not found. Target 'dfu_flash' is unavailable.")
endif()
