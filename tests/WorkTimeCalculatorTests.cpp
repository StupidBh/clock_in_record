#include "Attendance/WorkTimeCalculator.h"

#include <iostream>

namespace {
    int failures = 0;

    void expectEqual(const char* name, int actual, int expected)
    {
        if (actual == expected) {
            return;
        }

        std::cerr << name << ": expected " << expected << ", got " << actual << '\n';
        failures++;
    }

    void expectTrue(const char* name, bool value)
    {
        if (value) {
            return;
        }

        std::cerr << name << ": expected true\n";
        failures++;
    }

    void expectFalse(const char* name, bool value)
    {
        if (!value) {
            return;
        }

        std::cerr << name << ": expected false\n";
        failures++;
    }

    void testDefaultSchedule()
    {
        AttendanceRecord record;
        const WorkTimeResult result = WorkTimeCalculator::calculateWorkTimeResult(record);

        expectEqual("default actual work", result.actualWorkMinutes, 480);
        expectEqual("default standard work", result.standardWorkMinutes, 480);
        expectEqual("default breaks", result.totalBreakMinutes, 60);
        expectEqual("default overtime", result.overtimeMinutes, 0);
        expectEqual("default missing work", result.missingWorkMinutes, 0);
    }

    void testEarlyAndLateOvertime()
    {
        AttendanceRecord record;
        record.arrivalTime = QTime(8, 0);
        record.departureTime = QTime(19, 0);

        const WorkTimeResult result = WorkTimeCalculator::calculateWorkTimeResult(record);
        expectEqual("early and late overtime", result.overtimeMinutes, 90);
        expectEqual("early and late missing work", result.missingWorkMinutes, 0);
        expectEqual("early and late breaks", result.totalBreakMinutes, 90);
    }

    void testMissingWorkAndOvertimeRemainSeparate()
    {
        AttendanceRecord record;
        record.arrivalTime = QTime(10, 0);
        record.departureTime = QTime(19, 0);

        const WorkTimeResult result = WorkTimeCalculator::calculateWorkTimeResult(record);
        expectEqual("separate overtime", result.overtimeMinutes, 30);
        expectEqual("separate missing work", result.missingWorkMinutes, 60);

        const WorkTimeResult unchanged = WorkTimeCalculator::applyOvertimeOffset(result, false);
        expectEqual("disabled offset overtime", unchanged.overtimeMinutes, 30);
        expectEqual("disabled offset missing work", unchanged.missingWorkMinutes, 60);

        const WorkTimeResult offset = WorkTimeCalculator::applyOvertimeOffset(result, true);
        expectEqual("enabled offset overtime", offset.overtimeMinutes, 0);
        expectEqual("enabled offset missing work", offset.missingWorkMinutes, 30);

        record.departureTime = QTime(20, 0);
        const WorkTimeResult overtimeRemains = WorkTimeCalculator::applyOvertimeOffset(WorkTimeCalculator::calculateWorkTimeResult(record), true);
        expectEqual("enabled offset remaining overtime", overtimeRemains.overtimeMinutes, 30);
        expectEqual("enabled offset no missing work", overtimeRemains.missingWorkMinutes, 0);

        record.departureTime = QTime(23, 0);
        const WorkTimeResult largeOvertime = WorkTimeCalculator::calculateWorkTimeResult(record);
        expectEqual("large overtime before offset", largeOvertime.overtimeMinutes, 270);
        expectEqual("missing work before large offset", largeOvertime.missingWorkMinutes, 60);

        const WorkTimeResult largeOvertimeRemains = WorkTimeCalculator::applyOvertimeOffset(largeOvertime, true);
        expectEqual("large overtime after offset", largeOvertimeRemains.overtimeMinutes, 210);
        expectEqual("missing work after large offset", largeOvertimeRemains.missingWorkMinutes, 0);
    }

    void testLateArrivalAndEarlyDeparture()
    {
        AttendanceRecord record;
        record.arrivalTime = QTime(10, 0);
        record.departureTime = QTime(17, 0);

        const WorkTimeResult result = WorkTimeCalculator::calculateWorkTimeResult(record);
        expectEqual("late arrival", result.lateMinutes, 60);
        expectEqual("early departure", result.earlyLeaveMinutes, 60);
        expectEqual("late and early overtime", result.overtimeMinutes, 0);
        expectEqual("late and early missing work", result.missingWorkMinutes, 120);
    }

    void testBreakOverlapAcrossWorkEnd()
    {
        AttendanceRecord record;
        record.arrivalTime = QTime(8, 30);
        record.departureTime = QTime(19, 0);
        record.dinnerBreakStart = QTime(17, 30);
        record.dinnerBreakEnd = QTime(18, 30);

        const WorkTimeResult result = WorkTimeCalculator::calculateWorkTimeResult(record);
        expectEqual("crossing break standard work", result.standardWorkMinutes, 450);
        expectEqual("crossing break overtime", result.overtimeMinutes, 60);
        expectEqual("crossing break missing work", result.missingWorkMinutes, 0);
    }

    void testInvalidRanges()
    {
        AttendanceRecord invalidAttendance;
        invalidAttendance.departureTime = invalidAttendance.arrivalTime;
        expectFalse("equal attendance range", WorkTimeCalculator::hasValidAttendanceRange(invalidAttendance));

        const WorkTimeResult empty = WorkTimeCalculator::calculateWorkTimeResult(invalidAttendance);
        expectEqual("invalid attendance actual work", empty.actualWorkMinutes, 0);
        expectEqual("invalid attendance overtime", empty.overtimeMinutes, 0);
        expectEqual("invalid attendance missing work", empty.missingWorkMinutes, 0);

        AttendanceRecord overlappingBreaks;
        overlappingBreaks.dinnerBreakStart = QTime(13, 0);
        overlappingBreaks.dinnerBreakEnd = QTime(14, 0);
        expectFalse("overlapping breaks", WorkTimeCalculator::hasValidSchedule(overlappingBreaks));

        AttendanceRecord valid;
        expectTrue("default attendance range", WorkTimeCalculator::hasValidAttendanceRange(valid));
        expectTrue("default schedule", WorkTimeCalculator::hasValidSchedule(valid));
    }
} // namespace

int main()
{
    testDefaultSchedule();
    testEarlyAndLateOvertime();
    testMissingWorkAndOvertimeRemainSeparate();
    testLateArrivalAndEarlyDeparture();
    testBreakOverlapAcrossWorkEnd();
    testInvalidRanges();
    return failures == 0 ? 0 : 1;
}
