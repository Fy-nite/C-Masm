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
        src/mni/strings/strings.cpp
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


# Install targets
install(TARGETS microasm_static microasm_shared masm
                ARCHIVE DESTINATION lib
                LIBRARY DESTINATION lib
                RUNTIME DESTINATION bin)

install(FILES  src/debug_macros.h src/microasm_compiler.h src/operand_types.h src/common_defs.h src/debug_macros.h
                DESTINATION include/microasm)

# Print status message
message(STATUS "Configured MicroASM Toolchain build with static and shared libraries.")