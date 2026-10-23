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
        expectEqual("zero minutes", AttendanceFormatter::formatMinutes(0), u"0分钟"_s);
        expectEqual("whole hours", AttendanceFormatter::formatMinutes(120), u"2小时"_s);
    }
} // namespace

int main()
{
    testMonthlyCalculation();
    testFormatting();
    return failures == 0 ? 0 : 1;
}
