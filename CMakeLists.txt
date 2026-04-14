cmake_minimum_required(VERSION 3.16)
project(Compilador VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 COMPONENTS Widgets REQUIRED)

file(GLOB SOURCES
    src/main.cpp
    src/ide/*.cpp
    src/gals/*.cpp
    src/interpreter/*.cpp
)
file(GLOB HEADERS
    src/ide/*.h
    src/gals/*.h
    src/interpreter/*.h
)

add_executable(compilador ${SOURCES} ${HEADERS})
target_link_libraries(compilador Qt5::Widgets)
target_include_directories(compilador PRIVATE src src/gals)
