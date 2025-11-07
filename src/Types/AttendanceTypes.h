#ifndef ATTENDANCETYPES_H
#define ATTENDANCETYPES_H

#include <QTime>
#include <qDebug>

// 打卡记录结构体
struct AttendanceRecord
{
    bool needAverageCal;    // 是否加入平均加班计算
    QTime arrivalTime;      // 到达公司时间
    QTime departureTime;    // 离开公司时间
    QTime workStartTime;    // 标准上班时间
    QTime workEndTime;      // 标准下班时间
    QTime lunchBreakStart;  // 午餐开始时间
    QTime lunchBreakEnd;    // 午餐结束时间
    QTime dinnerBreakStart; // 晚餐开始时间
    QTime dinnerBreakEnd;   // 晚餐结束时间

    AttendanceRecord()
    {
        needAverageCal = true;
        arrivalTime = QTime(9, 0);
        departureTime = QTime(18, 0);
        workStartTime = QTime(9, 0);
        workEndTime = QTime(18, 0);
        lunchBreakStart = QTime(12, 30);
        lunchBreakEnd = QTime(13, 30);
        dinnerBreakStart = QTime(18, 0);
        dinnerBreakEnd = QTime(18, 30);
    }

    void print()
    {
        qDebug() << QString(
                        "==================\n "
                        "needAverageCal: %1\n "
                        "arrivalTime: %2\n "
                        "departureTime: %3\n "
                        "workStartTime: %4\n "
                        "workEndTime: %5\n "
                        "lunchBreakStart: %6\n "
                        "lunchBreakEnd: %7\n "
                        "dinnerBreakStart: %8\n "
                        "dinnerBreakEnd: %9\n "
                        "===================\n ")
                        .arg(needAverageCal)
                        .arg(arrivalTime.toString())
                        .arg(departureTime.toString())
                        .arg(workStartTime.toString())
                        .arg(workEndTime.toString())
                        .arg(lunchBreakStart.toString())
                        .arg(lunchBreakEnd.toString())
                        .arg(dinnerBreakStart.toString())
                        .arg(dinnerBreakEnd.toString())
                        .toUtf8()
                        .constData();
    }
};

// 工作时间计算结果
struct WorkTimeResult
{
    int actualWorkMinutes = 0;   // 实际工作时间（分钟）
    int standardWorkMinutes = 0; // 标准工作时间（分钟）
    int lateMinutes = 0;         // 迟到时间（分钟）
    int lateSize = 0;
    int earlyLeaveMinutes = 0;   // 早退时间（分钟）
    int earlyLeaveSize = 0;
    int overtimeMinutes = 0;     // 加班时间（分钟）
    int totalBreakMinutes = 0;   // 总休息时间（分钟）
};

#endif // ATTENDANCETYPES_H
