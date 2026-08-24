#pragma once
#include <QColor>
#include <Qt>

class QApplication;
class QPalette;

namespace AttendanceTheme {
    void apply(QApplication& application, Qt::ColorScheme colorScheme);

    [[nodiscard]] QColor attendanceBackground(const QPalette& palette, bool excludedFromAverage);
    [[nodiscard]] QColor attendanceForeground(const QPalette& palette);
    [[nodiscard]] QColor weekendForeground(const QPalette& palette);
}
