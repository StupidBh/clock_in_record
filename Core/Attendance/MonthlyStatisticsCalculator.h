#pragma once

#include "Attendance/AttendanceTypes.h"

#include <span>

struct MonthlyStatistics
{
    int workDays = 0;
    int overtimeMinutes = 0;
    int missingWorkMinutes = 0;
    int mealSubsidyCount = 0;
};

namespace MonthlyStatisticsCalculator {
    [[nodiscard]] MonthlyStatistics
        calculate(std::span<const AttendanceRecord> records, bool mealSubsidyEnabled, bool overtimeOffsetsMissingWork);
}
