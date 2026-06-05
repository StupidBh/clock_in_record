#include "WorkTimeCalculator.h"

WorkTimeResult WorkTimeCalculator::calculateWorkTimeResult(const AttendanceRecord& record)
{
    WorkTimeResult result;

    // 防护：到达时间晚于或等于离开时间时，返回全零结果
    if (!record.arrivalTime.isValid() || !record.departureTime.isValid() ||
        record.arrivalTime >= record.departureTime) {
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

    // 计算在公司总时间
    int totalMinutesAtWork = record.arrivalTime.secsTo(record.departureTime) / 60;

    // 计算实际休息时间
    result.totalBreakMinutes = 0;

    // 午餐时间
    if (isTimeRangeOverlap(record.arrivalTime, record.departureTime, record.lunchBreakStart, record.lunchBreakEnd)) {
        QTime lunchStart = maxTime(record.arrivalTime, record.lunchBreakStart);
        QTime lunchEnd = minTime(record.departureTime, record.lunchBreakEnd);
        if (lunchStart < lunchEnd) {
            result.totalBreakMinutes += lunchStart.secsTo(lunchEnd) / 60;
        }
    }

    // 晚餐时间
    if (isTimeRangeOverlap(record.arrivalTime, record.departureTime, record.dinnerBreakStart, record.dinnerBreakEnd)) {
        QTime dinnerStart = maxTime(record.arrivalTime, record.dinnerBreakStart);
        QTime dinnerEnd = minTime(record.departureTime, record.dinnerBreakEnd);
        if (dinnerStart < dinnerEnd) {
            result.totalBreakMinutes += dinnerStart.secsTo(dinnerEnd) / 60;
        }
    }

    // 实际工作时间 = 在公司时间 - 休息时间
    result.actualWorkMinutes = totalMinutesAtWork - result.totalBreakMinutes;

    // 标准工作时间
    int standardTotalMinutes = record.workStartTime.secsTo(record.workEndTime) / 60;
    int standardBreakMinutes = 0;

    // 标准午餐时间（与实际休息时间使用相同的 overlap + clip 模式）
    if (isTimeRangeOverlap(record.workStartTime, record.workEndTime, record.lunchBreakStart, record.lunchBreakEnd)) {
        QTime lunchStart = maxTime(record.workStartTime, record.lunchBreakStart);
        QTime lunchEnd = minTime(record.workEndTime, record.lunchBreakEnd);
        if (lunchStart < lunchEnd) {
            standardBreakMinutes += lunchStart.secsTo(lunchEnd) / 60;
        }
    }

    // 标准晚餐时间（同模式，处理跨工作起点的部分重叠）
    if (isTimeRangeOverlap(record.workStartTime, record.workEndTime, record.dinnerBreakStart, record.dinnerBreakEnd)) {
        QTime dinnerStart = maxTime(record.workStartTime, record.dinnerBreakStart);
        QTime dinnerEnd = minTime(record.workEndTime, record.dinnerBreakEnd);
        if (dinnerStart < dinnerEnd) {
            standardBreakMinutes += dinnerStart.secsTo(dinnerEnd) / 60;
        }
    }

    result.standardWorkMinutes = standardTotalMinutes - standardBreakMinutes;

    // 加班时间
    result.overtimeMinutes = result.actualWorkMinutes - result.standardWorkMinutes;

    return result;
}

bool WorkTimeCalculator::isTimeRangeOverlap(const QTime& start1,
                                            const QTime& end1,
                                            const QTime& start2,
                                            const QTime& end2)
{
    return start1 < end2 && start2 < end1;
}

QTime WorkTimeCalculator::maxTime(const QTime& time1, const QTime& time2)
{
    return time1 > time2 ? time1 : time2;
}

QTime WorkTimeCalculator::minTime(const QTime& time1, const QTime& time2)
{
    return time1 < time2 ? time1 : time2;
}
