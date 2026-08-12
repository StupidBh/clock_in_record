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
        ${CPP_RESOURCES}
        ${HEADER_RESOURCES}
        ${APP_RESOURCES}
        ${WIN_RESOURCES}
)

# 设置包含目录
target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# 链接 Qt 库
target_link_libraries(${PROJECT_NAME}
        Qt6::Widgets
        Qt6::Network
)

# 仅在 Windows 平台下且非静态编译时执行
if (WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
            WIN32_EXECUTABLE TRUE
    )

    # 自动查找当前构建环境匹配的 windeployqt 路径
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${Qt6_DIR}/../../../bin")
    if (WINDEPLOYQT_EXECUTABLE)
        # 添加构建后自定义命令
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${WINDEPLOYQT_EXECUTABLE} $<TARGET_FILE:${PROJECT_NAME}>
                COMMENT "Deploying Qt dependency DLLs to the output directory..."
        )
    else ()
        message(WARNING "windeployqt was not found; Qt dependency DLLs cannot be deployed automatically.")
    endif ()
endif ()
