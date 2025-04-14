cmake_minimum_required(VERSION 3.10)

# Project Name
project(MicroASM_Toolchain CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add the compiler executable
add_executable(microasm_compiler microasm_compiler.cpp)

# Add the interpreter executable
add_executable(microasm_interpreter microasm_interpreter.cpp)

# Optional: Add install rules if needed
# install(TARGETS microasm_compiler microasm_interpreter DESTINATION bin)

# Print status message
message(STATUS "Configured MicroASM Compiler and Interpreter build.")

