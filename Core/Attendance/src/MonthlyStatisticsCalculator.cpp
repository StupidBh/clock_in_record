#include "Attendance/MonthlyStatisticsCalculator.h"

#include "Attendance/WorkTimeCalculator.h"

namespace MonthlyStatisticsCalculator {
    MonthlyStatistics calculate(const std::span<const AttendanceRecord> records,
                                const bool mealSubsidyEnabled,
                                const bool overtimeOffsetsMissingWork)
    {
        MonthlyStatistics statistics;

        for (const AttendanceRecord& record : records) {
            const bool hasValidAttendance = WorkTimeCalculator::hasValidAttendanceRange(record);
            if (mealSubsidyEnabled && hasValidAttendance && record.departureTime >= record.mealSubsidyTime) {
                ++statistics.mealSubsidyCount;
            }

            if (!hasValidAttendance || !WorkTimeCalculator::hasValidSchedule(record)) {
                continue;
            }

            const WorkTimeResult result = WorkTimeCalculator::calculateWorkTimeResult(record);
            if (record.needAverageCal) {
                ++statistics.workDays;
            }
            statistics.overtimeMinutes += result.overtimeMinutes;
            statistics.missingWorkMinutes += result.missingWorkMinutes;
        }

        WorkTimeResult totals;
        totals.overtimeMinutes = statistics.overtimeMinutes;
        totals.missingWorkMinutes = statistics.missingWorkMinutes;
        totals = WorkTimeCalculator::applyOvertimeOffset(totals, overtimeOffsetsMissingWork);
        statistics.overtimeMinutes = totals.overtimeMinutes;
        statistics.missingWorkMinutes = totals.missingWorkMinutes;

        return statistics;
    }
} // namespace MonthlyStatisticsCalculator
