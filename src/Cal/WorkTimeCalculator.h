#ifndef WORKTIMECALCULATOR_H
#define WORKTIMECALCULATOR_H

#include "AttendanceTypes.h"
#include <QTime>

// 工作时间计算工具类
class WorkTimeCalculator {
public:
    static WorkTimeResult calculateWorkTimeResult(const AttendanceRecord& record);

private:
    // 辅助函数
    static bool isTimeRangeOverlap(const QTime& start1, const QTime& end1,
        const QTime& start2, const QTime& end2);
    static QTime maxTime(const QTime& time1, const QTime& time2);
    static QTime minTime(const QTime& time1, const QTime& time2);
};

#endif // WORKTIMECALCULATOR_H
