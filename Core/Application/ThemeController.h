#pragma once
#include "Application/Theme.h"

#include <QObject>

class QApplication;

class ThemeController final : public QObject {
    Q_OBJECT

public:
    explicit ThemeController(QApplication& application, QObject* parent = nullptr);

    [[nodiscard]] AttendanceTheme::Mode mode() const;
    void setMode(AttendanceTheme::Mode mode);

signals:
    void modeChanged(AttendanceTheme::Mode mode);

private:
    void apply();
    void saveMode() const;

    QApplication& m_application;
    AttendanceTheme::Mode m_mode = AttendanceTheme::Mode::Auto;
};
