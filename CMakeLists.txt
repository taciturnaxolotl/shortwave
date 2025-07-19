cmake_minimum_required(VERSION 3.16)
project(ShortwaveApp CXX)

# Generate compile_commands.json for editor support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Target Windows XP compatibility
add_definitions(-DWINVER=0x0501)
add_definitions(-D_WIN32_WINNT=0x0501)
add_definitions(-D_WIN32_WINNT_WIN2K=0x0500)
add_definitions(-DMINGW_HAS_SECURE_API=1)

# Disable threading to avoid mcfgthread dependency
add_compile_options(-fno-threadsafe-statics)
add_compile_options(-D_GLIBCXX_HAS_GTHREADS=0)

add_executable(ShortwaveApp WIN32 main.cpp)

# Add BASS library from libs directory
target_include_directories(ShortwaveApp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ShortwaveApp user32 gdi32 winmm wininet ${CMAKE_CURRENT_SOURCE_DIR}/libs/bass.lib)

# Include current directory for headers
target_include_directories(ShortwaveApp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Minimal static linking and avoid threading dependencies
target_link_options(ShortwaveApp PRIVATE 
    -static-libgcc 
    -static-libstdc++
    -static
    -Wl,--subsystem,windows:5.01
)

# Configurable output directory
if(DEFINED OUTPUT_DIR)
    set_target_properties(ShortwaveApp PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_DIR}"
    )
endif()

set_target_properties(ShortwaveApp PROPERTIES
    OUTPUT_NAME "Shortwave"
)

# Configurable output directory
if(DEFINED OUTPUT_DIR)
    set_target_properties(ShortwaveApp PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_DIR}"
    )
endif()