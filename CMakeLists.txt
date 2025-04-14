cmake_minimum_required(VERSION 3.10)

# Project Name
project(MicroASM_Toolchain CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add source files
set(SOURCES
    microasm_compiler.cpp
    microasm_interpreter.cpp
)

# Create a static library
add_library(microasm_lib STATIC ${SOURCES})

# Create the main executable using the same source files
add_executable(masm main.cpp ${SOURCES})

# Install targets (optional)
install(TARGETS microasm_lib
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib)


install(FILES microasm_compiler.h microasm_interpreter.h operand_types.h common_defs.h debug_macros.h
        DESTINATION include/microasm)

# Print status message
message(STATUS "Configured MicroASM Toolchain build with static library.")
