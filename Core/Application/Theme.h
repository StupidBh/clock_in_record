#pragma once
#include <QColor>
#include <Qt>

class QApplication;
class QPalette;

namespace AttendanceTheme {
    enum class Mode
    {
        Auto,
        Light,
        Dark,
    };

    void apply(QApplication& application, Mode mode, Qt::ColorScheme colorScheme);
    void apply(QApplication& application, Qt::ColorScheme colorScheme);

    [[nodiscard]] bool resolvesToDark(QApplication& application, Mode mode, Qt::ColorScheme colorScheme);

    [[nodiscard]] QColor attendanceBackground(const QPalette& palette, bool excludedFromAverage);
    [[nodiscard]] QColor attendanceForeground(const QPalette& palette);
    [[nodiscard]] QColor weekendForeground(const QPalette& palette);
} // namespace AttendanceTheme
