# 自动收集源文件
file(GLOB_RECURSE APP_SOURCES 
    "*.cpp"
    "Utils/*.cpp"
    "Cal/*.cpp"
)
file(GLOB_RECURSE APP_HEADERS 
    "*.h"
    "Utils/*.h"
    "Cal/*.h"
)

# 收集类型定义头文件
file(GLOB_RECURSE TYPE_HEADERS "Types/*.h")

# 资源文件配置
set(RESOURCE_DIR "${CMAKE_SOURCE_DIR}/resources")
set(APP_RESOURCES "${RESOURCE_DIR}/resources.qrc")

# 创建可执行文件
add_executable(${PROJECT_NAME}
    ${APP_SOURCES}
    ${TYPE_HEADERS}
    ${APP_HEADERS}
    ${APP_RESOURCES}
)
# 设置包含目录
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}          # src目录
    ${CMAKE_CURRENT_SOURCE_DIR}/Types    # Types目录
    ${CMAKE_CURRENT_SOURCE_DIR}/Utils    # Utils目录
    ${CMAKE_CURRENT_SOURCE_DIR}/Cal      # Cal目录
)
# 链接Qt库
target_link_libraries(${PROJECT_NAME}
    Qt${QT_VERSION_MAJOR}::Core 
    Qt${QT_VERSION_MAJOR}::Widgets
)

# Windows特定设置
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()
