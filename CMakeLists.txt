cmake_minimum_required(VERSION 4.0)
project(AttendanceApp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- Output directories: bin/<Debug|Release>/ at project root ----
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/$<CONFIG>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/$<CONFIG>")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/$<CONFIG>")

if (MSVC)
    add_compile_options(/utf-8)
    add_compile_options(/Zc:__cplusplus)
    set(CMAKE_PREFIX_PATH "D:/Qt6/6.11.1/msvc2022_64")
elseif (MINGW)
    set(CMAKE_PREFIX_PATH "D:/Qt6/6.11.1/mingw_64")
endif ()

# 查找Qt库
find_package(QT NAMES Qt6 REQUIRED COMPONENTS Core Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets Network)

# Qt MOC设置
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_subdirectory(Core)
