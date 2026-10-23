#include "Attendance/AttendanceFormatter.h"

using namespace Qt::StringLiterals;

namespace AttendanceFormatter {
    QString formatMinutes(const int minutes)
    {
        const int absoluteMinutes = minutes < 0 ? -minutes : minutes;
        const QString sign = minutes < 0 ? u"-"_s : QString();
        const int hours = absoluteMinutes / 60;
        const int remainingMinutes = absoluteMinutes % 60;
        if (hours == 0) {
            return u"%1%2分钟"_s.arg(sign).arg(remainingMinutes);
        }
        if (remainingMinutes == 0) {
            return u"%1%2小时"_s.arg(sign).arg(hours);
        }
        return u"%1%2小时%3分钟"_s.arg(sign).arg(hours).arg(remainingMinutes);
    }
} // namespace AttendanceFormatter
