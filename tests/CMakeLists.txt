file(GLOB ATTENDANCE_TEST_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)
file(GLOB ATTENDANCE_TEST_HEADERS CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

file(GLOB ATTENDANCE_APP_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/Core/Application/src/*.cpp"
        "${CMAKE_SOURCE_DIR}/Core/Attendance/src/*.cpp"
        "${CMAKE_SOURCE_DIR}/Core/Calendar/src/*.cpp"
        "${CMAKE_SOURCE_DIR}/Core/Settings/src/*.cpp"
        "${CMAKE_SOURCE_DIR}/Core/Widgets/src/*.cpp"
)
file(GLOB ATTENDANCE_APP_HEADERS CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/Core/Application/*.h"
        "${CMAKE_SOURCE_DIR}/Core/Attendance/*.h"
        "${CMAKE_SOURCE_DIR}/Core/Calendar/*.h"
        "${CMAKE_SOURCE_DIR}/Core/Settings/*.h"
        "${CMAKE_SOURCE_DIR}/Core/Widgets/*.h"
)
list(FILTER ATTENDANCE_APP_SOURCES EXCLUDE REGEX "/Main\\.cpp$")

foreach (ATTENDANCE_TEST_SOURCE IN LISTS ATTENDANCE_TEST_SOURCES)
    get_filename_component(ATTENDANCE_TEST_TARGET "${ATTENDANCE_TEST_SOURCE}" NAME_WE)

    add_executable(${ATTENDANCE_TEST_TARGET}
            "${ATTENDANCE_TEST_SOURCE}"
            ${ATTENDANCE_TEST_HEADERS}
            ${ATTENDANCE_APP_SOURCES}
            ${ATTENDANCE_APP_HEADERS}
    )

    target_include_directories(${ATTENDANCE_TEST_TARGET} PRIVATE
            ${CMAKE_SOURCE_DIR}/Core
    )

    target_link_libraries(${ATTENDANCE_TEST_TARGET} PRIVATE
            Qt6::Widgets
    )

    add_test(NAME ${ATTENDANCE_TEST_TARGET} COMMAND ${ATTENDANCE_TEST_TARGET})
    set_tests_properties(${ATTENDANCE_TEST_TARGET} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=windows")
endforeach ()
