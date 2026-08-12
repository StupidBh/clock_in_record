#pragma once
#include <QTime>

// 打卡记录结构体
struct AttendanceRecord
{
    bool needAverageCal = true;       // 是否加入平均加班计算
    QTime arrivalTime { 9, 0 };       // 到达公司时间
    QTime departureTime { 18, 0 };    // 离开公司时间
    QTime workStartTime { 9, 0 };     // 标准上班时间
    QTime workEndTime { 18, 0 };      // 标准下班时间
    QTime lunchBreakStart { 12, 30 }; // 午餐开始时间
    QTime lunchBreakEnd { 13, 30 };   // 午餐结束时间
    QTime dinnerBreakStart { 18, 0 }; // 晚餐开始时间
    QTime dinnerBreakEnd { 18, 30 };  // 晚餐结束时间
    QTime mealSubsidyTime { 21, 0 };  // 餐补起算时间
};

// 工作时间计算结果
struct WorkTimeResult
{
    int actualWorkMinutes = 0;   // 实际工作时间（分钟）
    int standardWorkMinutes = 0; // 标准工作时间（分钟）
    int lateMinutes = 0;         // 迟到时间（分钟）
    int earlyLeaveMinutes = 0;   // 早退时间（分钟）
    int overtimeMinutes = 0;     // 标准工作时段外的工作时间（分钟）
    int missingWorkMinutes = 0;  // 标准工作时段内缺少的工作时间（分钟）
    int totalBreakMinutes = 0;   // 总休息时间（分钟）
};
