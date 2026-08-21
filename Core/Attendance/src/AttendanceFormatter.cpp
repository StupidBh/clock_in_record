#include "Attendance/AttendanceFormatter.h"

using namespace Qt::StringLiterals;

namespace AttendanceFormatter {
    QString formatMinutes(const int minutes)
    {
        const int absoluteMinutes = minutes < 0 ? -minutes : minutes;
        const QString sign = minutes < 0 ? u"-"_s : QString();
        return u"%1%2小时%3分钟"_s.arg(sign).arg(absoluteMinutes / 60).arg(absoluteMinutes % 60);
    }

    QString formatDailyResult(const WorkTimeResult& result)
    {
        QString resultText;

        if (result.lateMinutes > 0) {
            resultText += u"[迟到] %1\n"_s.arg(formatMinutes(result.lateMinutes));
        }
        if (result.earlyLeaveMinutes > 0) {
            resultText += u"[早退] %1\n"_s.arg(formatMinutes(result.earlyLeaveMinutes));
        }

        resultText += u"[标准工时] %1\n\n"_s.arg(formatMinutes(result.standardWorkMinutes));
        resultText += u"[实际工时] %1\n"_s.arg(formatMinutes(result.actualWorkMinutes));

        if (result.totalBreakMinutes > 0) {
            resultText += u"[休息时间] %1\n"_s.arg(formatMinutes(result.totalBreakMinutes));
        }
        if (result.overtimeMinutes > 0) {
            resultText += u"[加班时间] %1\n"_s.arg(formatMinutes(result.overtimeMinutes));
        }
        if (result.missingWorkMinutes > 0) {
            resultText += u"[缺少标准工时] %1"_s.arg(formatMinutes(result.missingWorkMinutes));
        }
        if (result.overtimeMinutes == 0 && result.missingWorkMinutes == 0) {
            resultText += u"[今日无缺]"_s;
        }

        return resultText;
    }

    QString formatMonthlySummary(const QDate& month,
                                 const MonthlyStatistics& statistics,
                                 const int targetMinutesPerDay,
                                 const bool mealSubsidyEnabled)
    {
        QString summary = u"统计月份: %1年%2月\n"_s.arg(month.year()).arg(month.month());
        summary += u"工作天数: %1天\n"_s.arg(statistics.workDays);

        const int targetOvertimeMinutes = targetMinutesPerDay * statistics.workDays;
        if (targetMinutesPerDay == 0) {
            summary += u"总加班时长: %1\n"_s.arg(formatMinutes(statistics.overtimeMinutes));
        }
        else {
            if (statistics.workDays > 0) {
                const double averageOvertimeHours = statistics.overtimeMinutes / (60.0 * statistics.workDays);
                summary += u"均加班时间: %1小时\n"_s.arg(averageOvertimeHours, 0, 'f', 3);
            }
            if (statistics.overtimeMinutes < targetOvertimeMinutes) {
                summary += u"缺加班时间: %1\n"_s.arg(formatMinutes(targetOvertimeMinutes - statistics.overtimeMinutes));
            }
            else if (statistics.overtimeMinutes > targetOvertimeMinutes) {
                summary += u"余加班时间: %1\n"_s.arg(formatMinutes(statistics.overtimeMinutes - targetOvertimeMinutes));
            }
        }

        if (statistics.missingWorkMinutes > 0) {
            summary += u"缺少标准工时: %1\n"_s.arg(formatMinutes(statistics.missingWorkMinutes));
        }
        if (mealSubsidyEnabled) {
            summary += u"餐补次数: %1"_s.arg(statistics.mealSubsidyCount);
        }

        return summary;
    }
} // namespace AttendanceFormatter
