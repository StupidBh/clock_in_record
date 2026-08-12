#include "Attendance/WorkTimeCalculator.h"

bool WorkTimeCalculator::hasValidAttendanceRange(const AttendanceRecord& record)
{
    return record.arrivalTime.isValid() && record.departureTime.isValid() && record.arrivalTime < record.departureTime;
}

bool WorkTimeCalculator::hasValidSchedule(const AttendanceRecord& record)
{
    auto isValidRange = [](const QTime& start, const QTime& end) {
        return start.isValid() && end.isValid() && start < end;
    };

    if (!isValidRange(record.workStartTime, record.workEndTime) ||
        !isValidRange(record.lunchBreakStart, record.lunchBreakEnd) ||
        !isValidRange(record.dinnerBreakStart, record.dinnerBreakEnd)) {
        return false;
    }

    return !isTimeRangeOverlap(record.lunchBreakStart,
                               record.lunchBreakEnd,
                               record.dinnerBreakStart,
                               record.dinnerBreakEnd);
}

WorkTimeResult WorkTimeCalculator::calculateWorkTimeResult(const AttendanceRecord& record)
{
    WorkTimeResult result;

    if (!hasValidAttendanceRange(record) || !hasValidSchedule(record)) {
        return result;
    }

    // 计算迟到时间
    if (record.arrivalTime > record.workStartTime) {
        result.lateMinutes = record.workStartTime.secsTo(record.arrivalTime) / 60;
    }

    // 计算早退时间
    if (record.departureTime < record.workEndTime) {
        result.earlyLeaveMinutes = record.departureTime.secsTo(record.workEndTime) / 60;
    }

    result.totalBreakMinutes = calculateBreakMinutes(record, record.arrivalTime, record.departureTime);
    result.actualWorkMinutes = calculateWorkingMinutes(record, record.arrivalTime, record.departureTime);
    result.standardWorkMinutes = calculateWorkingMinutes(record, record.workStartTime, record.workEndTime);

    const QTime standardAttendanceStart = maxTime(record.arrivalTime, record.workStartTime);
    const QTime standardAttendanceEnd = minTime(record.departureTime, record.workEndTime);
    const int standardAttendanceMinutes =
        calculateWorkingMinutes(record, standardAttendanceStart, standardAttendanceEnd);
    result.missingWorkMinutes = result.standardWorkMinutes - standardAttendanceMinutes;

    const QTime earlyOvertimeEnd = minTime(record.departureTime, record.workStartTime);
    const QTime lateOvertimeStart = maxTime(record.arrivalTime, record.workEndTime);
    result.overtimeMinutes = calculateWorkingMinutes(record, record.arrivalTime, earlyOvertimeEnd) +
                             calculateWorkingMinutes(record, lateOvertimeStart, record.departureTime);

    return result;
}

WorkTimeResult WorkTimeCalculator::applyOvertimeOffset(WorkTimeResult result, bool enabled)
{
    if (enabled) {
        const int offsetMinutes = qMin(result.overtimeMinutes, result.missingWorkMinutes);
        result.overtimeMinutes -= offsetMinutes;
        result.missingWorkMinutes -= offsetMinutes;
    }

    return result;
}

bool WorkTimeCalculator::isTimeRangeOverlap(const QTime& start1,
                                            const QTime& end1,
                                            const QTime& start2,
                                            const QTime& end2)
{
    return start1 < end2 && start2 < end1;
}

int WorkTimeCalculator::calculateBreakMinutes(const AttendanceRecord& record, const QTime& start, const QTime& end)
{
    if (!start.isValid() || !end.isValid() || start >= end) {
        return 0;
    }

    int breakMinutes = 0;
    auto addOverlap = [&](const QTime& breakStart, const QTime& breakEnd) {
        if (!isTimeRangeOverlap(start, end, breakStart, breakEnd)) {
            return;
        }

        const QTime overlapStart = maxTime(start, breakStart);
        const QTime overlapEnd = minTime(end, breakEnd);
        breakMinutes += overlapStart.secsTo(overlapEnd) / 60;
    };

    addOverlap(record.lunchBreakStart, record.lunchBreakEnd);
    addOverlap(record.dinnerBreakStart, record.dinnerBreakEnd);
    return breakMinutes;
}

int WorkTimeCalculator::calculateWorkingMinutes(const AttendanceRecord& record, const QTime& start, const QTime& end)
{
    if (!start.isValid() || !end.isValid() || start >= end) {
        return 0;
    }

    return start.secsTo(end) / 60 - calculateBreakMinutes(record, start, end);
}

QTime WorkTimeCalculator::maxTime(const QTime& time1, const QTime& time2)
{
    return time1 > time2 ? time1 : time2;
}

QTime WorkTimeCalculator::minTime(const QTime& time1, const QTime& time2)
{
    return time1 < time2 ? time1 : time2;
}
