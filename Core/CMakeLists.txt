# 自动收集各功能目录中的源文件和头文件
file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB_RECURSE APP_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

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
        ${APP_SOURCES}
        ${APP_HEADERS}
        ${APP_RESOURCES}
        ${WIN_RESOURCES}  # 添加Windows资源文件
)

# 设置包含目录
target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# 链接Qt库
target_link_libraries(${PROJECT_NAME}
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Network
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
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${Qt6_DIR}/../../../bin" "${Qt5_DIR}/../../../bin")
    if (WINDEPLOYQT_EXECUTABLE)
        # 添加构建后自定义命令
        add_custom_command(TARGET AttendanceApp POST_BUILD
                COMMAND ${WINDEPLOYQT_EXECUTABLE} $<TARGET_FILE:AttendanceApp>
                COMMENT "正在自动部署 Qt 依赖 DLL 到输出目录..."
        )
    else ()
        message(WARNING "未找到 windeployqt 程序，无法自动部署 DLL。")
    endif ()
endif ()
