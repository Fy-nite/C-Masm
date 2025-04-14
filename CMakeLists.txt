cmake_minimum_required(VERSION 3.10)

# Project Name
project(MicroASM_Toolchain CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add the unified executable
add_executable(masm main.cpp microasm_compiler.cpp microasm_interpreter.cpp)

# Optional: Add install rules if needed
# install(TARGETS microasm_toolchain DESTINATION bin)

# Print status message
message(STATUS "Configured MicroASM Toolchain build.")

