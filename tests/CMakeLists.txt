add_executable(WorkTimeCalculatorTests
        WorkTimeCalculatorTests.cpp
        ${CMAKE_SOURCE_DIR}/Core/Attendance/src/WorkTimeCalculator.cpp
)

target_include_directories(WorkTimeCalculatorTests PRIVATE
        ${CMAKE_SOURCE_DIR}/Core
)

target_link_libraries(WorkTimeCalculatorTests PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
)

add_test(NAME WorkTimeCalculatorTests COMMAND WorkTimeCalculatorTests)

add_executable(AttendanceSettingsTests
        AttendanceSettingsTests.cpp
        ${CMAKE_SOURCE_DIR}/Core/Application/AttendanceMainWindow.h
        ${CMAKE_SOURCE_DIR}/Core/Application/src/AttendanceMainWindow.cpp
        ${CMAKE_SOURCE_DIR}/Core/Attendance/src/WorkTimeCalculator.cpp
        ${CMAKE_SOURCE_DIR}/Core/Calendar/CustomCalendarWidget.h
        ${CMAKE_SOURCE_DIR}/Core/Calendar/src/CustomCalendarWidget.cpp
        ${CMAKE_SOURCE_DIR}/Core/Settings/TimeSettingDialog.h
        ${CMAKE_SOURCE_DIR}/Core/Settings/src/TimeSettingDialog.cpp
        ${CMAKE_SOURCE_DIR}/Core/Widgets/CollapsibleGroupBox.h
        ${CMAKE_SOURCE_DIR}/Core/Widgets/src/CollapsibleGroupBox.cpp
)

target_include_directories(AttendanceSettingsTests PRIVATE
        ${CMAKE_SOURCE_DIR}/Core
)

target_link_libraries(AttendanceSettingsTests PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Widgets
)

add_test(NAME AttendanceSettingsTests COMMAND AttendanceSettingsTests)
set_tests_properties(AttendanceSettingsTests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=windows")
