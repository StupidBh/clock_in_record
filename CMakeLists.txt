cmake_minimum_required(VERSION 4.3)
project(AttendanceApp VERSION 1.0.0 LANGUAGES CXX)

include(CTest)

# ---- Output directories: bin/<Debug|Release>/ at project root ----
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/$<CONFIG>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/$<CONFIG>")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/$<CONFIG>")

# 查找 Qt 库
find_package(Qt6 6.11 REQUIRED COMPONENTS Core Widgets Network)

qt_standard_project_setup()

add_library(attendance_project_options INTERFACE)
target_compile_features(attendance_project_options INTERFACE cxx_std_23)
target_compile_options(attendance_project_options INTERFACE
        "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/W4;/permissive->"
)

add_subdirectory(Core)

if (BUILD_TESTING)
    add_subdirectory(tests)
endif ()
