file(GLOB_RECURSE ATTENDANCE_CALCULATION_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/Attendance/*.cpp"
)
file(GLOB_RECURSE ATTENDANCE_CALCULATION_HEADERS CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/Attendance/*.h"
)

add_library(attendance_calculation STATIC
        ${ATTENDANCE_CALCULATION_SOURCES}
        ${ATTENDANCE_CALCULATION_HEADERS}
)
add_library(Attendance::Calculation ALIAS attendance_calculation)

target_include_directories(attendance_calculation PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(attendance_calculation PUBLIC
        attendance_project_options
        Qt6::Core
)

file(GLOB_RECURSE ATTENDANCE_UI_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/Application/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Calendar/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Settings/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Widgets/*.cpp"
)
file(GLOB_RECURSE ATTENDANCE_UI_HEADERS CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/Application/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/Calendar/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/Settings/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/Widgets/*.h"
)

set(ATTENDANCE_ENTRYPOINT_SOURCES ${ATTENDANCE_UI_SOURCES})
list(FILTER ATTENDANCE_ENTRYPOINT_SOURCES INCLUDE REGEX "/Main\\.cpp$")
list(FILTER ATTENDANCE_UI_SOURCES EXCLUDE REGEX "/Main\\.cpp$")

list(LENGTH ATTENDANCE_ENTRYPOINT_SOURCES ATTENDANCE_ENTRYPOINT_COUNT)
if (NOT ATTENDANCE_ENTRYPOINT_COUNT EQUAL 1)
    message(FATAL_ERROR "Expected exactly one Main.cpp entry point, found ${ATTENDANCE_ENTRYPOINT_COUNT}")
endif ()

add_library(attendance_ui STATIC
        ${ATTENDANCE_UI_SOURCES}
        ${ATTENDANCE_UI_HEADERS}
)
add_library(Attendance::Ui ALIAS attendance_ui)

target_include_directories(attendance_ui PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(attendance_ui PUBLIC
        Attendance::Calculation
        Qt6::Widgets
)

# 资源文件配置
set(RESOURCE_DIR "${CMAKE_SOURCE_DIR}/resources")
set(APP_RESOURCES "${RESOURCE_DIR}/resources.qrc")

# Windows特定资源文件处理
set(WIN_RESOURCES "")
if (WIN32)
    # 创建资源文件
    set(RC_FILE "${CMAKE_CURRENT_BINARY_DIR}/app_icon.rc")
    set(ICON_PATH "${RESOURCE_DIR}/Icons/logo.ico")

    # 确保图标文件存在
    if (EXISTS "${ICON_PATH}")
        # 将路径转换为Windows格式并正确转义
        file(TO_NATIVE_PATH "${ICON_PATH}" NATIVE_ICON_PATH)
        string(REPLACE "\\" "\\\\" ESCAPED_ICON_PATH "${NATIVE_ICON_PATH}")

        file(WRITE ${RC_FILE} "
#include <windows.h>
IDI_ICON1 ICON \"${ESCAPED_ICON_PATH}\"
")
        set(WIN_RESOURCES ${RC_FILE})
    else ()
        message(WARNING "Icon file not found: ${ICON_PATH}")
    endif ()
endif ()

# 创建可执行文件
add_executable(${PROJECT_NAME}
        ${ATTENDANCE_ENTRYPOINT_SOURCES}
        ${APP_RESOURCES}
        ${WIN_RESOURCES}
)

# 链接Qt库
target_link_libraries(${PROJECT_NAME}
        Attendance::Ui
        Qt6::Network
)

# Windows特定设置
if (WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
            WIN32_EXECUTABLE TRUE
    )
endif ()


# 仅在 Windows 平台下且非静态编译时执行
if (WIN32)
    # 自动查找当前构建环境匹配的 windeployqt 路径
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${Qt6_DIR}/../../../bin")
    if (WINDEPLOYQT_EXECUTABLE)
        # 添加构建后自定义命令
        add_custom_command(TARGET AttendanceApp POST_BUILD
                COMMAND ${WINDEPLOYQT_EXECUTABLE} $<TARGET_FILE:AttendanceApp>
                COMMENT "Deploying Qt dependency DLLs to the output directory..."
        )
    else ()
        message(WARNING "windeployqt was not found; Qt dependency DLLs cannot be deployed automatically.")
    endif ()
endif ()
