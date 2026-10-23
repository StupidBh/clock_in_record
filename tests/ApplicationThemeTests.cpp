#include "Application/AttendanceMainWindow.h"
#include "Application/Theme.h"
#include "Application/ThemeController.h"
#include "Calendar/CustomCalendarWidget.h"

#include <QApplication>
#include <QColor>
#include <QLabel>
#include <QMenu>
#include <QPalette>
#include <QSettings>
#include <QTableView>
#include <QTemporaryDir>
#include <QTextCharFormat>
#include <QToolButton>
#include <algorithm>
#include <cmath>
#include <iostream>

using namespace Qt::StringLiterals;

namespace {
    int failures = 0;

    void expectTrue(const char* name, const bool value)
    {
        if (value) {
            return;
        }

        std::cerr << name << ": expected true\n";
        failures++;
    }

    [[nodiscard]] double linearChannel(const int channel)
    {
        const double value = channel / 255.0;
        return value <= 0.04045 ? value / 12.92 : std::pow((value + 0.055) / 1.055, 2.4);
    }

    [[nodiscard]] double luminance(const QColor& color)
    {
        return 0.2126 * linearChannel(color.red()) + 0.7152 * linearChannel(color.green()) +
               0.0722 * linearChannel(color.blue());
    }

    [[nodiscard]] double contrastRatio(const QColor& first, const QColor& second)
    {
        const double lighter = std::max(luminance(first), luminance(second));
        const double darker = std::min(luminance(first), luminance(second));
        return (lighter + 0.05) / (darker + 0.05);
    }

    void expectContrast(const char* name, const QColor& foreground, const QColor& background)
    {
        const double contrast = contrastRatio(foreground, background);
        if (contrast >= 4.5) {
            return;
        }

        std::cerr << name << ": expected contrast >= 4.5, got " << contrast << " (" << foreground.name().toStdString()
                  << " on " << background.name().toStdString() << ")\n";
        failures++;
    }

    void verifyLabelContrast(const char* name, const QString& objectName, const QColor& windowBackground)
    {
        QLabel label;
        label.setObjectName(objectName);
        label.ensurePolished();
        QColor background = label.palette().color(QPalette::Window);
        if (background.alpha() == 0) {
            background = windowBackground;
        }
        expectContrast(name, label.palette().color(QPalette::WindowText), background);
    }

    void verifyTheme(QApplication& application, const Qt::ColorScheme colorScheme)
    {
        AttendanceTheme::apply(application, colorScheme);
        const QPalette palette = application.palette();
        const QString arrowVariant = colorScheme == Qt::ColorScheme::Dark ? u"dark"_s : u"light"_s;

        expectTrue("theme selects matching spin up arrow",
                   application.styleSheet().contains(u"spin-arrow-up-%1.svg"_s.arg(arrowVariant)));
        expectTrue("theme selects matching spin down arrow",
                   application.styleSheet().contains(u"spin-arrow-down-%1.svg"_s.arg(arrowVariant)));

        expectContrast("window text", palette.color(QPalette::WindowText), palette.color(QPalette::Window));
        expectContrast("surface text", palette.color(QPalette::Text), palette.color(QPalette::Base));
        expectContrast("selected text", palette.color(QPalette::HighlightedText), palette.color(QPalette::Highlight));

        const QColor attendanceForeground = AttendanceTheme::attendanceForeground(palette);
        expectContrast("attendance", attendanceForeground, AttendanceTheme::attendanceBackground(palette, false));
        expectContrast("excluded attendance",
                       attendanceForeground,
                       AttendanceTheme::attendanceBackground(palette, true));
        expectContrast("weekend", AttendanceTheme::weekendForeground(palette), palette.color(QPalette::Base));

        const QColor windowBackground = palette.color(QPalette::Window);
        verifyLabelContrast("statistics value", u"statsWorkDaysValueLabel"_s, windowBackground);
        verifyLabelContrast("statistics period", u"statisticsPeriodLabel"_s, windowBackground);
        verifyLabelContrast("calculation result label", u"calculationResultLabel"_s, windowBackground);
        verifyLabelContrast("calculation detail", u"calculationResultDetailLabel"_s, windowBackground);
        verifyLabelContrast("settings status", u"settingsStatusLabel"_s, windowBackground);
        verifyLabelContrast("settings error label", u"settingsErrorLabel"_s, windowBackground);

        CustomCalendarWidget calendar;
        calendar.ensurePolished();
        expectContrast("calendar weekend format",
                       calendar.weekdayTextFormat(Qt::Saturday).foreground().color(),
                       palette.color(QPalette::Base));
        auto* tableView = calendar.findChild<QTableView*>();
        if (!tableView) {
            std::cerr << "calendar table: expected internal table view\n";
            failures++;
            return;
        }
        tableView->ensurePolished();
        expectContrast("calendar",
                       tableView->palette().color(QPalette::Text),
                       tableView->palette().color(QPalette::Base));
    }

    void testThemeModes(QApplication& application)
    {
        using namespace Qt::StringLiterals;
        QSettings settings;
        settings.clear();
        settings.sync();
        settings.setValue(u"appearance/themeMode"_s, u"invalid"_s);
        settings.sync();
        ThemeController invalidController(application);
        expectTrue("invalid theme mode falls back to auto", invalidController.mode() == AttendanceTheme::Mode::Auto);
        settings.clear();
        settings.sync();

        expectTrue("light mode overrides dark system",
                   !AttendanceTheme::resolvesToDark(application, AttendanceTheme::Mode::Light, Qt::ColorScheme::Dark));
        expectTrue("dark mode overrides light system",
                   AttendanceTheme::resolvesToDark(application, AttendanceTheme::Mode::Dark, Qt::ColorScheme::Light));

        ThemeController controller(application);
        expectTrue("theme mode defaults to auto", controller.mode() == AttendanceTheme::Mode::Auto);
        controller.setMode(AttendanceTheme::Mode::Dark);
        expectTrue("dark mode is persisted", settings.value(u"appearance/themeMode"_s).toString() == u"dark"_s);
        controller.setMode(AttendanceTheme::Mode::Auto);
        expectTrue("auto mode is persisted", settings.value(u"appearance/themeMode"_s).toString() == u"auto"_s);

        AttendanceMainWindow window(&controller);
        auto* const themeButton = window.findChild<QToolButton*>(u"themeButton"_s);
        expectTrue("theme button exists", themeButton != nullptr);
        if (!themeButton || !themeButton->menu()) {
            return;
        }

        expectTrue("auto mode is initially checked", themeButton->menu()->actions().at(2)->isChecked());
        themeButton->menu()->actions().at(0)->trigger();
        expectTrue("light menu changes mode", controller.mode() == AttendanceTheme::Mode::Light);
        expectTrue("light menu updates button", themeButton->text() == u"主题：浅色"_s);
        expectTrue("light mode is checked", themeButton->menu()->actions().at(0)->isChecked());

        themeButton->menu()->actions().at(1)->trigger();
        expectTrue("dark menu changes mode", controller.mode() == AttendanceTheme::Mode::Dark);
        expectTrue("dark menu updates button", themeButton->text() == u"主题：深色"_s);

        themeButton->menu()->actions().at(2)->trigger();
        expectTrue("auto menu changes mode", controller.mode() == AttendanceTheme::Mode::Auto);
        expectTrue("auto menu updates button", themeButton->text() == u"主题：自动"_s);
    }
} // namespace

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);
    QCoreApplication::setOrganizationName(u"AttendanceAppTests"_s);
    QCoreApplication::setApplicationName(u"ApplicationThemeTests"_s);
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QTemporaryDir settingsDirectory;
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDirectory.path());

    testThemeModes(application);
    verifyTheme(application, Qt::ColorScheme::Light);
    verifyTheme(application, Qt::ColorScheme::Dark);

    return failures == 0 ? 0 : 1;
}
