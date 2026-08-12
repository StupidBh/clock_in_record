file(GLOB CPP_RESOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/Application/src/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Attendance/src/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Calendar/src/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Settings/src/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Widgets/src/*.cpp"
)

file(GLOB HEADER_RESOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/Application/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/Attendance/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/Calendar/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/Settings/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/Widgets/*.h"
)

set(ENTRYPOINT_RESOURCES ${CPP_RESOURCES})
list(FILTER ENTRYPOINT_RESOURCES INCLUDE REGEX "/Main\\.cpp$")
list(FILTER CPP_RESOURCES EXCLUDE REGEX "/Main\\.cpp$")

list(LENGTH ENTRYPOINT_RESOURCES ENTRYPOINT_COUNT)
if (NOT ENTRYPOINT_COUNT EQUAL 1)
    message(FATAL_ERROR "Expected exactly one Main.cpp entry point, found ${ENTRYPOINT_COUNT}")
endif ()

add_library(${PROJECT_NAME}Core STATIC
        ${CPP_RESOURCES}
        ${HEADER_RESOURCES}
)
add_library(Attendance::Core ALIAS ${PROJECT_NAME}Core)

target_include_directories(${PROJECT_NAME}Core PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
target_compile_options(${PROJECT_NAME}Core PUBLIC
        "$<$<COMPILE_LANG_AND_ID:C,MSVC>:/W4>"
        "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/W4;/permissive->"
)
target_link_libraries(${PROJECT_NAME}Core PUBLIC
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
        ${ENTRYPOINT_RESOURCES}
        ${APP_RESOURCES}
        ${WIN_RESOURCES}
)

# 链接Qt库
target_link_libraries(${PROJECT_NAME}
        Attendance::Core
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
