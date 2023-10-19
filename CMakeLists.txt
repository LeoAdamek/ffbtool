cmake_minimum_required(VERSION 3.27)
project(ffbtool LANGUAGES CXX VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_TOOLCHAIN_FILE "I:\\Programs\\Utilities\\vcpkg\\scripts\\buildsystems\\vcpkg.cmake" CACHE STRING "Vcpkg Toolchain File")

file(
    GLOB 
    FFBTOOL_SOURCES 
    "src/ui/ui.cxx"
    "src/ui/imgui/imgui.cpp"
    "src/ui/imgui/imgui_draw.cpp"
    "src/ui/imgui/imgui_widgets.cpp"
    "src/ui/imgui/imgui_tables.cpp"
    "src/ui/imgui/imgui_demo.cpp"
    "src/*.cxx"
)

if (WIN32)
    # Add the Win32 platform-specific implementations
    list(APPEND FFBTOOL_SOURCES "src/ui/ui_win32.cxx" "src/ui/imgui/imgui_impl_dx11.cpp" "src/ui/imgui/imgui_impl_win32.cpp")

endif ()

add_executable(ffbtool ${FFBTOOL_SOURCES})

find_package(hidapi CONFIG REQUIRED)
target_link_libraries(ffbtool PRIVATE hidapi::hidapi hidapi::include)

find_package(fmt CONFIG REQUIRED)
target_link_libraries(ffbtool PRIVATE fmt::fmt)

find_package(Boost REQUIRED COMPONENTS graph)
target_link_libraries(ffbtool PRIVATE Boost::boost Boost::graph)

if (WIN32) 
    # Add the winAPI adapter for HIDAPI
    target_link_libraries(ffbtool PRIVATE hidapi::winapi)

    # Add the required packages for windows rendering backend
endif()
