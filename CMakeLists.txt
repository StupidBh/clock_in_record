cmake_minimum_required(VERSION 4.0)
project(AttendanceApp VERSION 1.0.0 LANGUAGES C CXX)

include(CTest)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---- Output directories: bin/<Debug|Release>/ at project root ----
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/$<CONFIG>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/$<CONFIG>")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/$<CONFIG>")

if (MSVC AND NOT Qt6_DIR AND NOT CMAKE_PREFIX_PATH)
    set(CMAKE_PREFIX_PATH "D:/Qt6/6.11.1/msvc2022_64")
elseif (MINGW AND NOT Qt6_DIR AND NOT CMAKE_PREFIX_PATH)
    set(CMAKE_PREFIX_PATH "D:/Qt6/6.11.1/mingw_64")
endif ()

# 查找 Qt 库
find_package(Qt6 6.11 REQUIRED COMPONENTS Core Widgets Network)

qt_standard_project_setup()

add_library(attendance_project_options INTERFACE)
target_compile_features(attendance_project_options INTERFACE c_std_17 cxx_std_20)
target_compile_options(attendance_project_options INTERFACE
        "$<$<COMPILE_LANG_AND_ID:C,MSVC>:/utf-8>"
        "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/utf-8>"
        "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/Zc:__cplusplus>"
)

add_subdirectory(Core)

if (BUILD_TESTING)
    add_subdirectory(tests)
endif ()
