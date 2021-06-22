cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

project(pico_examples C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the SDK
pico_sdk_init()

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
        )

add_executable(weatherstation
        weatherstation.c
        )

# Pull in our pico_stdlib which pulls in commonly used features
target_link_libraries(weatherstation pico_stdlib)

# create map/bin/hex file etc.
pico_add_extra_outputs(weatherstation)

# add url via pico_set_program_url
pico_set_program_url(weatherstation "https://github.com/jknutson/pico-weatherstation-c")
