add_library(attendance_core STATIC)
add_library(Attendance::Core ALIAS attendance_core)

target_sources(attendance_core
        PRIVATE
        Application/src/AttendanceMainWindow.cpp
        Application/src/SingleInstance.cpp
        Application/src/Theme.cpp
        Attendance/src/AttendanceFormatter.cpp
        Attendance/src/MonthlyStatisticsCalculator.cpp
        Attendance/src/WorkTimeCalculator.cpp
        Calendar/src/CustomCalendarWidget.cpp
        Settings/src/AttendanceSettings.cpp
        Settings/src/TimeSettingDialog.cpp
        Widgets/src/CollapsibleGroupBox.cpp
        PUBLIC
        FILE_SET public_headers
        TYPE HEADERS
        BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
        FILES
        Application/AttendanceMainWindow.h
        Application/SingleInstance.h
        Application/Theme.h
        Attendance/AttendanceFormatter.h
        Attendance/AttendanceTypes.h
        Attendance/MonthlyStatisticsCalculator.h
        Attendance/WorkTimeCalculator.h
        Calendar/CustomCalendarWidget.h
        Settings/AttendanceSettings.h
        Settings/TimeSettingDialog.h
        Widgets/CollapsibleGroupBox.h
)

target_link_libraries(attendance_core PUBLIC
        attendance_project_options
        Qt6::Network
        Qt6::Widgets
)

set_target_properties(attendance_core PROPERTIES
        CXX_EXTENSIONS OFF
)

set(RESOURCE_DIR "${CMAKE_SOURCE_DIR}/resources")
qt_add_resources(APP_RESOURCE_SOURCES "${RESOURCE_DIR}/resources.qrc")

set(APP_PLATFORM_SOURCES "")
if (WIN32)
    set(ICON_PATH "${RESOURCE_DIR}/Icons/logo.ico")
    if (EXISTS "${ICON_PATH}")
        cmake_path(NATIVE_PATH ICON_PATH NORMALIZE NATIVE_ICON_PATH)
        string(REPLACE "\\" "\\\\" ESCAPED_ICON_PATH "${NATIVE_ICON_PATH}")
        configure_file(
                "${RESOURCE_DIR}/app_icon.rc.in"
                "${CMAKE_CURRENT_BINARY_DIR}/app_icon.rc"
                @ONLY
        )
        list(APPEND APP_PLATFORM_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/app_icon.rc")
    else ()
        message(WARNING "Icon file not found: ${ICON_PATH}")
    endif ()
endif ()

qt_add_executable(${PROJECT_NAME} WIN32
        Application/src/Main.cpp
        ${APP_RESOURCE_SOURCES}
        ${APP_PLATFORM_SOURCES}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
        Attendance::Core
        Qt6::Network
)

set_target_properties(${PROJECT_NAME} PROPERTIES
        CXX_EXTENSIONS OFF
)

if (WIN32)
    if (TARGET Qt6::windeployqt)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND "$<TARGET_FILE:Qt6::windeployqt>"
                "$<IF:$<CONFIG:Debug>,--debug,--release>"
                --no-compiler-runtime
                "$<TARGET_FILE:${PROJECT_NAME}>"
                COMMENT "Deploying Qt dependency DLLs to the output directory..."
                VERBATIM
        )
    else ()
        message(WARNING "The current Qt package does not provide windeployqt; Qt dependency DLLs cannot be deployed automatically.")
    endif ()
endif ()
