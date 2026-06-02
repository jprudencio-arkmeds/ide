cmake_minimum_required(VERSION 3.16)
project(Compilador VERSION 1.0 LANGUAGES CXX)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT compilador)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

foreach(CONFIG Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER ${CONFIG} CONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_SOURCE_DIR}/bin/${CONFIG})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_SOURCE_DIR}/bin/${CONFIG})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_SOURCE_DIR}/bin/${CONFIG})
endforeach()

# Fallback for single-config generators (Linux Make/Ninja)
if(NOT CMAKE_CONFIGURATION_TYPES)
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE Debug)
    endif()
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})
endif()

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
		
		add_executable(compilador WIN32 ${GUI_SOURCES} ${HEADERS} ${GUI_HEADERS})
		
    target_link_libraries(compilador Qt6::Widgets gals)
    target_include_directories(compilador PRIVATE src)
else()
    message(STATUS "Qt6 not found - building CLI only (compilador-cmd)")
endif()
