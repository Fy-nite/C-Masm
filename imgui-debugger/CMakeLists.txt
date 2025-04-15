cmake_minimum_required(VERSION 3.10)
project(masm_imgui_debugger CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ImGui sources (adjust if your structure is different)
set(IMGUI_SOURCES
    imgui.cpp
    imgui_draw.cpp
    imgui_tables.cpp
    imgui_widgets.cpp
    imgui_impl_sdl2.cpp
    imgui_impl_sdlrenderer2.cpp
    imstb_truetype.h
)

# Main debugger source
set(SOURCES
    masm_debugger.cpp
    ${IMGUI_SOURCES}
)

# MicroASM interpreter include dir (adjust if needed)
include_directories(
    ${CMAKE_SOURCE_DIR}/..
    ${CMAKE_SOURCE_DIR}/../ # For microasm_interpreter.h etc.
    .
)

# Find SDL2 (use system package or vcpkg, etc.)
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

# Add MicroASM interpreter sources (adjust as needed)
set(MICROASM_SOURCES
    ../microasm_interpreter.cpp
    
)

add_executable(masm_debugger ${SOURCES} ${MICROASM_SOURCES})

target_link_libraries(masm_debugger PRIVATE ${SDL2_LIBRARIES})

# If MicroASM interpreter is a static lib, link it here:
# target_link_libraries(masm_debugger PRIVATE microasm_interpreter)

# Optionally, copy shaders or font files if needed
