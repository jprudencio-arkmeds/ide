cmake_minimum_required(VERSION 3.16)
project(Compilador VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(CLI_ONLY "Build command line compiler without Qt" OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

if (CLI_ONLY)
    message(STATUS "Building CLI-only version without Qt")
    file(GLOB SOURCES
        src/main_cli.cpp
        src/Compilation.cpp
        src/gals/*.cpp
    )
else()
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)

    find_package(Qt6 COMPONENTS Widgets REQUIRED)

    file(GLOB SOURCES
        src/main.cpp
        src/ide/*.cpp
        src/Compilation.cpp
        src/gals/*.cpp
    )
endif()

file(GLOB HEADERS
    src/Compilation.h
    src/ide/*.h
    src/gals/*.h
)

add_executable(compilador ${SOURCES} ${HEADERS})

if (NOT CLI_ONLY)
    target_link_libraries(compilador Qt6::Widgets)
endif()

target_include_directories(compilador PRIVATE src src/gals)
