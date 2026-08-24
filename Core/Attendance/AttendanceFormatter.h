#pragma once
#include "Attendance/AttendanceTypes.h"
#include "Attendance/MonthlyStatisticsCalculator.h"

#include <QDate>
#include <QString>

namespace AttendanceFormatter {
    [[nodiscard]] QString formatMinutes(int minutes);
    [[nodiscard]] QString formatDailyResult(const WorkTimeResult& result);
    [[nodiscard]] QString formatMonthlySummary(const QDate& month,
                                               const MonthlyStatistics& statistics,
                                               int targetMinutesPerDay,
                                               bool mealSubsidyEnabled);
}
