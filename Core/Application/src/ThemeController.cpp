#include "Application/ThemeController.h"

#include <QApplication>
#include <QSettings>
#include <QStyleHints>

using namespace Qt::StringLiterals;

namespace {
    constexpr auto ThemeModeKey = "appearance/themeMode"_L1;

    AttendanceTheme::Mode loadMode()
    {
        QSettings settings;
        const QString value = settings.value(ThemeModeKey, u"auto"_s).toString().toLower();
        if (value == u"light"_s) {
            return AttendanceTheme::Mode::Light;
        }
        if (value == u"dark"_s) {
            return AttendanceTheme::Mode::Dark;
        }
        return AttendanceTheme::Mode::Auto;
    }

    QString modeValue(const AttendanceTheme::Mode mode)
    {
        switch (mode) {
        case AttendanceTheme::Mode::Light: return u"light"_s;
        case AttendanceTheme::Mode::Dark : return u"dark"_s;
        case AttendanceTheme::Mode::Auto : return u"auto"_s;
        }
        return u"auto"_s;
    }
} // namespace

ThemeController::ThemeController(QApplication& application, QObject* parent) :
    QObject(parent),
    m_application(application),
    m_mode(loadMode())
{
    apply();
    connect(m_application.styleHints(), &QStyleHints::colorSchemeChanged, this, [this](const Qt::ColorScheme) {
        if (m_mode == AttendanceTheme::Mode::Auto) {
            apply();
        }
    });
}

AttendanceTheme::Mode ThemeController::mode() const
{
    return m_mode;
}

void ThemeController::setMode(const AttendanceTheme::Mode mode)
{
    if (m_mode == mode) {
        return;
    }

    m_mode = mode;
    saveMode();
    apply();
    emit modeChanged(m_mode);
}

void ThemeController::apply()
{
    AttendanceTheme::apply(m_application, m_mode, m_application.styleHints()->colorScheme());
}

void ThemeController::saveMode() const
{
    QSettings settings;
    settings.setValue(ThemeModeKey, modeValue(m_mode));
    settings.sync();
}
