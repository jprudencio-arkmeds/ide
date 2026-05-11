cmake_minimum_required(VERSION 3.16)
project(Compilador VERSION 1.0 LANGUAGES CXX)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT compilador)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

find_package(Qt6 COMPONENTS Widgets QUIET)

# Shared library from gals
file(GLOB GALS_SOURCES src/gals/*.cpp)
file(GLOB GALS_HEADERS src/gals/*.h)

add_library(gals SHARED ${GALS_SOURCES} ${GALS_HEADERS})
target_include_directories(gals PUBLIC src/gals)
target_compile_definitions(gals PRIVATE GalsDefS)

# CLI executable (always built)
file(GLOB CLI_SOURCES
    src/main_cli.cpp
    src/Compilation.cpp
)

file(GLOB HEADERS
    src/Compilation.h
)

add_executable(compilador-cmd ${CLI_SOURCES} ${HEADERS})
target_include_directories(compilador-cmd PRIVATE src)
target_link_libraries(compilador-cmd gals)

# GUI executable (only if Qt6 is available)
if (Qt6Widgets_FOUND)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)

    file(GLOB GUI_SOURCES
        src/main.cpp
        src/ide/*.cpp
        src/Compilation.cpp
    )

    file(GLOB GUI_HEADERS
        src/ide/*.h
    )

    add_executable(compilador ${GUI_SOURCES} ${HEADERS} ${GUI_HEADERS})
    target_link_libraries(compilador Qt6::Widgets gals)
    target_include_directories(compilador PRIVATE src)
else()
    message(STATUS "Qt6 not found - building CLI only (compilador-cmd)")
endif()
