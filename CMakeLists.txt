cmake_minimum_required(VERSION 3.10)

# Project Name
project(MicroASM_Toolchain CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add source files (EXCLUDE microasm_decoder.cpp)
set(SOURCES
        src/microasm_compiler.cpp
        src/microasm_interpreter.cpp
        src/microasm_capi.cpp
        src/microasm_decoder.cpp
)

# Create a static library
add_library(microasm_static STATIC ${SOURCES})
set_target_properties(microasm_static PROPERTIES OUTPUT_NAME microasm)

# Create a shared library
add_library(microasm_shared SHARED ${SOURCES})
set_target_properties(microasm_shared PROPERTIES OUTPUT_NAME microasm)

# Create the main executable
add_executable(masm src/main.cpp)
target_link_libraries(masm microasm_static)


# --- Raylib integration for decoder ---
find_package(raylib REQUIRED)
# --------------------------------------

# Install targets
install(TARGETS microasm_static microasm_shared
                ARCHIVE DESTINATION lib
                LIBRARY DESTINATION lib
                RUNTIME DESTINATION bin)

install(FILES microasm_compiler.h microasm_interpreter.h operand_types.h common_defs.h debug_macros.h
                DESTINATION include/microasm)

# Print status message
message(STATUS "Configured MicroASM Toolchain build with static and shared libraries.")
