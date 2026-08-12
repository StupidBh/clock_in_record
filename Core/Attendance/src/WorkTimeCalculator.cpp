#include "Attendance/WorkTimeCalculator.h"

#include <algorithm>
#include <array>

namespace {
    struct TimeRange
    {
        QTime start;
        QTime end;
    };

    bool rangesOverlap(const TimeRange& first, const TimeRange& second)
    {
        return first.start < second.end && second.start < first.end;
    }

    QTime laterTime(const QTime& first, const QTime& second)
    {
        return first > second ? first : second;
    }

    QTime earlierTime(const QTime& first, const QTime& second)
    {
        return first < second ? first : second;
    }

    std::array<TimeRange, 2> breakRanges(const AttendanceRecord& record)
    {
        return { TimeRange { .start = record.lunchBreakStart, .end = record.lunchBreakEnd },
                 TimeRange { .start = record.dinnerBreakStart, .end = record.dinnerBreakEnd } };
    }

    int calculateBreakMinutes(const AttendanceRecord& record, const TimeRange& range)
    {
        if (!range.start.isValid() || !range.end.isValid() || range.start >= range.end) {
            return 0;
        }

        int breakMinutes = 0;
        for (const auto& breakRange : breakRanges(record)) {
            if (!rangesOverlap(range, breakRange)) {
                continue;
            }

            const QTime overlapStart = laterTime(range.start, breakRange.start);
            const QTime overlapEnd = earlierTime(range.end, breakRange.end);
            breakMinutes += overlapStart.secsTo(overlapEnd) / 60;
        }
        return breakMinutes;
    }

    int calculateWorkingMinutes(const AttendanceRecord& record, const TimeRange& range)
    {
        if (!range.start.isValid() || !range.end.isValid() || range.start >= range.end) {
            return 0;
        }

        return range.start.secsTo(range.end) / 60 - calculateBreakMinutes(record, range);
    }
} // namespace

namespace WorkTimeCalculator {
    bool hasValidAttendanceRange(const AttendanceRecord& record)
    {
        return record.arrivalTime.isValid() && record.departureTime.isValid() && record.arrivalTime < record.departureTime;
    }

    bool hasValidSchedule(const AttendanceRecord& record)
    {
        auto isValidRange = [](const QTime& start, const QTime& end) {
            return start.isValid() && end.isValid() && start < end;
        };

        if (!isValidRange(record.workStartTime, record.workEndTime) || !isValidRange(record.lunchBreakStart, record.lunchBreakEnd) ||
            !isValidRange(record.dinnerBreakStart, record.dinnerBreakEnd)) {
            return false;
        }

        return !rangesOverlap({ .start = record.lunchBreakStart, .end = record.lunchBreakEnd },
                              { .start = record.dinnerBreakStart, .end = record.dinnerBreakEnd });
    }

    WorkTimeResult calculateWorkTimeResult(const AttendanceRecord& record)
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

        const TimeRange attendanceRange { .start = record.arrivalTime, .end = record.departureTime };
        result.totalBreakMinutes = calculateBreakMinutes(record, attendanceRange);
        result.actualWorkMinutes = calculateWorkingMinutes(record, attendanceRange);
        result.standardWorkMinutes = calculateWorkingMinutes(record, { .start = record.workStartTime, .end = record.workEndTime });

        const QTime standardAttendanceStart = laterTime(record.arrivalTime, record.workStartTime);
        const QTime standardAttendanceEnd = earlierTime(record.departureTime, record.workEndTime);
        const int standardAttendanceMinutes = calculateWorkingMinutes(record, { .start = standardAttendanceStart, .end = standardAttendanceEnd });
        result.missingWorkMinutes = result.standardWorkMinutes - standardAttendanceMinutes;

        const QTime earlyOvertimeEnd = earlierTime(record.departureTime, record.workStartTime);
        const QTime lateOvertimeStart = laterTime(record.arrivalTime, record.workEndTime);
        result.overtimeMinutes = calculateWorkingMinutes(record, { .start = record.arrivalTime, .end = earlyOvertimeEnd }) +
                                 calculateWorkingMinutes(record, { .start = lateOvertimeStart, .end = record.departureTime });

        return result;
    }

    WorkTimeResult applyOvertimeOffset(WorkTimeResult result, bool enabled)
    {
        if (enabled) {
            const int offsetMinutes = std::min(result.overtimeMinutes, result.missingWorkMinutes);
            result.overtimeMinutes -= offsetMinutes;
            result.missingWorkMinutes -= offsetMinutes;
        }

        return result;
    }
} // namespace WorkTimeCalculator
