cmake_minimum_required(VERSION 3.16)
project(HelloWorldApp CXX)

# Generate compile_commands.json for editor support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Target Windows XP compatibility
add_definitions(-DWINVER=0x0501)
add_definitions(-D_WIN32_WINNT=0x0501)

add_executable(HelloWorldApp WIN32 main.cpp)
target_link_libraries(HelloWorldApp user32 gdi32)

# Minimal static linking
target_link_options(HelloWorldApp PRIVATE 
    -static-libgcc 
    -static-libstdc++
    -Wl,--subsystem,windows:5.01
)

# Configurable output directory
if(DEFINED OUTPUT_DIR)
    set_target_properties(HelloWorldApp PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_DIR}"
    )
endif()

set_target_properties(HelloWorldApp PROPERTIES
    OUTPUT_NAME "HelloWorld"
)

# Configurable output directory
if(DEFINED OUTPUT_DIR)
    set_target_properties(HelloWorldApp PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_DIR}"
    )
endif()