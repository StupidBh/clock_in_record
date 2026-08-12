#pragma once
#include "Attendance/AttendanceTypes.h"

namespace WorkTimeCalculator {
    [[nodiscard]] bool hasValidAttendanceRange(const AttendanceRecord& record);
    [[nodiscard]] bool hasValidSchedule(const AttendanceRecord& record);
    [[nodiscard]] WorkTimeResult calculateWorkTimeResult(const AttendanceRecord& record);
    [[nodiscard]] WorkTimeResult applyOvertimeOffset(WorkTimeResult result, bool enabled);
}
