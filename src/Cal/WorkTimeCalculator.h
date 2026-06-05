#pragma once
#include "AttendanceTypes.h"
#include <QSettings>

// 从 QSettings 读取全局默认时间设置
inline AttendanceRecord loadGlobalTimeDefaults()
{
    QSettings settings;
    AttendanceRecord defaults;
    auto readTime = [&](const QString& key, const QTime& fallback) {
        return QTime::fromString(settings.value(key, fallback.toString("hh:mm")).toString(), "hh:mm");
    };
    AttendanceRecord record;
    record.workStartTime = readTime("settings/workStart", defaults.workStartTime);
    record.workEndTime = readTime("settings/workEnd", defaults.workEndTime);
    record.lunchBreakStart = readTime("settings/lunchStart", defaults.lunchBreakStart);
    record.lunchBreakEnd = readTime("settings/lunchEnd", defaults.lunchBreakEnd);
    record.dinnerBreakStart = readTime("settings/dinnerStart", defaults.dinnerBreakStart);
    record.dinnerBreakEnd = readTime("settings/dinnerEnd", defaults.dinnerBreakEnd);
    return record;
}

// 工作时间计算工具类
class WorkTimeCalculator {
  public:
    static WorkTimeResult calculateWorkTimeResult(const AttendanceRecord& record);

  private:
    // 辅助函数
    static bool isTimeRangeOverlap(const QTime& start1, const QTime& end1, const QTime& start2, const QTime& end2);
    static QTime maxTime(const QTime& time1, const QTime& time2);
    static QTime minTime(const QTime& time1, const QTime& time2);
};
