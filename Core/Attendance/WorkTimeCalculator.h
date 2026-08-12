#pragma once
#include "Attendance/AttendanceTypes.h"
#include <QSettings>

// 从 QSettings 读取全局默认时间设置
inline AttendanceRecord loadGlobalTimeDefaults()
{
    QSettings settings;
    AttendanceRecord defaults;
    auto readTime = [&](const QString& key, const QTime& fallback) {
        QTime value = QTime::fromString(settings.value(key, fallback.toString("hh:mm")).toString(), "hh:mm");
        return value.isValid() ? value : fallback;
    };
    AttendanceRecord record;
    record.workStartTime = readTime("settings/workStart", defaults.workStartTime);
    record.workEndTime = readTime("settings/workEnd", defaults.workEndTime);
    record.lunchBreakStart = readTime("settings/lunchStart", defaults.lunchBreakStart);
    record.lunchBreakEnd = readTime("settings/lunchEnd", defaults.lunchBreakEnd);
    record.dinnerBreakStart = readTime("settings/dinnerStart", defaults.dinnerBreakStart);
    record.dinnerBreakEnd = readTime("settings/dinnerEnd", defaults.dinnerBreakEnd);
    record.mealSubsidyTime = readTime("settings/mealSubsidy", defaults.mealSubsidyTime);
    return record;
}

namespace WorkTimeCalculator {
    [[nodiscard]] bool hasValidAttendanceRange(const AttendanceRecord& record);
    [[nodiscard]] bool hasValidSchedule(const AttendanceRecord& record);
    [[nodiscard]] WorkTimeResult calculateWorkTimeResult(const AttendanceRecord& record);
    [[nodiscard]] WorkTimeResult applyOvertimeOffset(WorkTimeResult result, bool enabled);
}
