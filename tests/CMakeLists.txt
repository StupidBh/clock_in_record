add_executable(WorkTimeCalculatorTests
        WorkTimeCalculatorTests.cpp
)

target_link_libraries(WorkTimeCalculatorTests PRIVATE
        Attendance::Calculation
)

add_test(NAME WorkTimeCalculatorTests COMMAND WorkTimeCalculatorTests)

add_executable(AttendanceSettingsTests
        AttendanceSettingsTests.cpp
)

target_link_libraries(AttendanceSettingsTests PRIVATE
        Attendance::Ui
)

add_test(NAME AttendanceSettingsTests COMMAND AttendanceSettingsTests)
set_tests_properties(AttendanceSettingsTests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=windows")
