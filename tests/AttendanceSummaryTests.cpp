#include "Attendance/AttendanceFormatter.h"
#include "Attendance/MonthlyStatisticsCalculator.h"
#include "Attendance/WorkTimeCalculator.h"

#include <QString>

#include <array>
#include <iostream>

namespace {
    int failures = 0;

    void expectEqual(const char* name, const int actual, const int expected)
    {
        if (actual == expected) {
            return;
        }

        std::cerr << name << ": expected " << expected << ", got " << actual << '\n';
        ++failures;
    }

    void expectEqual(const char* name, const QString& actual, const QString& expected)
    {
        if (actual == expected) {
            return;
        }

        std::cerr << name << ": expected \"" << expected.toStdString() << "\", got \"" << actual.toStdString()
                  << "\"\n";
        ++failures;
    }

    void testMonthlyCalculation()
    {
        AttendanceRecord missingWork;
        missingWork.arrivalTime = QTime(10, 0);

        AttendanceRecord overtime;
        overtime.departureTime = QTime(23, 0);

        AttendanceRecord excludedFromAverage;
        excludedFromAverage.needAverageCal = false;
        excludedFromAverage.departureTime = QTime(19, 0);

        AttendanceRecord invalidSchedule;
        invalidSchedule.departureTime = QTime(22, 0);
        invalidSchedule.workEndTime = invalidSchedule.workStartTime;

        const std::array records { missingWork, overtime, excludedFromAverage, invalidSchedule };
        const MonthlyStatistics statistics = MonthlyStatisticsCalculator::calculate(records, true, true);

        expectEqual("monthly work days", statistics.workDays, 2);
        expectEqual("monthly offset overtime", statistics.overtimeMinutes, 240);
        expectEqual("monthly offset missing work", statistics.missingWorkMinutes, 0);
        expectEqual("monthly meal subsidies", statistics.mealSubsidyCount, 2);
    }

    void testFormatting()
    {
        using namespace Qt::StringLiterals;

        expectEqual("positive minutes", AttendanceFormatter::formatMinutes(90), u"1小时30分钟"_s);
        expectEqual("negative minutes", AttendanceFormatter::formatMinutes(-90), u"-1小时30分钟"_s);

        const WorkTimeResult dailyResult = WorkTimeCalculator::calculateWorkTimeResult(AttendanceRecord { });
        expectEqual("daily result",
                    AttendanceFormatter::formatDailyResult(dailyResult),
                    u"[标准工时] 8小时0分钟\n\n[实际工时] 8小时0分钟\n[休息时间] 1小时0分钟\n[今日无缺]"_s);

        const WorkTimeResult exceptionalDailyResult {
            .actualWorkMinutes = 390,
            .standardWorkMinutes = 480,
            .lateMinutes = 60,
            .earlyLeaveMinutes = 30,
            .overtimeMinutes = 30,
            .missingWorkMinutes = 90,
            .totalBreakMinutes = 60,
        };
        expectEqual(
            "exceptional daily result",
            AttendanceFormatter::formatDailyResult(exceptionalDailyResult),
            u"[迟到] 1小时0分钟\n[早退] 0小时30分钟\n[标准工时] 8小时0分钟\n\n[实际工时] 6小时30分钟\n[休息时间] 1小时0分钟\n[加班时间] 0小时30分钟\n[缺少标准工时] 1小时30分钟"_s);

        const MonthlyStatistics statistics {
            .workDays = 2,
            .overtimeMinutes = 210,
            .missingWorkMinutes = 0,
            .mealSubsidyCount = 1,
        };
        expectEqual("monthly result",
                    AttendanceFormatter::formatMonthlySummary(QDate(2026, 8, 1), statistics, 0, true),
                    u"统计月份: 2026年8月\n工作天数: 2天\n总加班时长: 3小时30分钟\n餐补次数: 1"_s);

        const MonthlyStatistics aboveTarget {
            .workDays = 2,
            .overtimeMinutes = 310,
            .missingWorkMinutes = 45,
        };
        expectEqual(
            "monthly above target",
            AttendanceFormatter::formatMonthlySummary(QDate(2026, 8, 1), aboveTarget, 150, false),
            u"统计月份: 2026年8月\n工作天数: 2天\n均加班时间: 2.583小时\n余加班时间: 0小时10分钟\n缺少标准工时: 0小时45分钟\n"_s);

        const MonthlyStatistics belowTarget {
            .workDays = 2,
            .overtimeMinutes = 290,
        };
        expectEqual("monthly below target",
                    AttendanceFormatter::formatMonthlySummary(QDate(2026, 8, 1), belowTarget, 150, false),
                    u"统计月份: 2026年8月\n工作天数: 2天\n均加班时间: 2.417小时\n缺加班时间: 0小时10分钟\n"_s);
    }
} // namespace

int main()
{
    testMonthlyCalculation();
    testFormatting();
    return failures == 0 ? 0 : 1;
}
