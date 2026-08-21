#include "Application/Theme.h"

#include <QApplication>
#include <QPalette>

#include <utility>

namespace {
    struct ThemeColors
    {
        QColor window;
        QColor surface;
        QColor raisedSurface;
        QColor border;
        QColor text;
        QColor mutedText;
        QColor accent;
        QColor accentHover;
        QColor accentPressed;
        QColor accentText;
        QColor error;
        QColor link;
    };

    [[nodiscard]] bool isDarkTheme(const QApplication& application, const Qt::ColorScheme colorScheme)
    {
        if (colorScheme == Qt::ColorScheme::Dark) {
            return true;
        }
        if (colorScheme == Qt::ColorScheme::Light) {
            return false;
        }

        return application.palette().color(QPalette::Window).lightness() < 128;
    }

    [[nodiscard]] ThemeColors colorsFor(const bool dark)
    {
        if (dark) {
            return {
                QColor("#1b1d20"),
                QColor("#25282c"),
                QColor("#30343a"),
                QColor("#464b52"),
                QColor("#f2f4f7"),
                QColor("#aeb4bc"),
                QColor("#176da8"),
                QColor("#2082c2"),
                QColor("#11557f"),
                QColor("#ffffff"),
                QColor("#ff7b86"),
                QColor("#72bff0"),
            };
        }

        return {
            QColor("#f4f6f8"),
            QColor("#ffffff"),
            QColor("#e9eef2"),
            QColor("#c8d0d8"),
            QColor("#20242a"),
            QColor("#5f6872"),
            QColor("#0b639b"),
            QColor("#0e78b8"),
            QColor("#084d79"),
            QColor("#ffffff"),
            QColor("#b4232d"),
            QColor("#075f96"),
        };
    }

    [[nodiscard]] QPalette createPalette(const ThemeColors& colors)
    {
        QPalette palette;
        palette.setColor(QPalette::Window, colors.window);
        palette.setColor(QPalette::WindowText, colors.text);
        palette.setColor(QPalette::Base, colors.surface);
        palette.setColor(QPalette::AlternateBase, colors.raisedSurface);
        palette.setColor(QPalette::ToolTipBase, colors.raisedSurface);
        palette.setColor(QPalette::ToolTipText, colors.text);
        palette.setColor(QPalette::Text, colors.text);
        palette.setColor(QPalette::Button, colors.raisedSurface);
        palette.setColor(QPalette::ButtonText, colors.text);
        palette.setColor(QPalette::BrightText, colors.accentText);
        palette.setColor(QPalette::Light, colors.raisedSurface.lighter(115));
        palette.setColor(QPalette::Midlight, colors.raisedSurface);
        palette.setColor(QPalette::Mid, colors.border);
        palette.setColor(QPalette::Dark, colors.border.darker(125));
        palette.setColor(QPalette::Shadow, colors.window.darker(150));
        palette.setColor(QPalette::Highlight, colors.accent);
        palette.setColor(QPalette::HighlightedText, colors.accentText);
        palette.setColor(QPalette::Link, colors.link);
        palette.setColor(QPalette::LinkVisited, colors.link);
        palette.setColor(QPalette::PlaceholderText, colors.mutedText);
        palette.setColor(QPalette::Accent, colors.accent);

        palette.setColor(QPalette::Disabled, QPalette::WindowText, colors.mutedText);
        palette.setColor(QPalette::Disabled, QPalette::Text, colors.mutedText);
        palette.setColor(QPalette::Disabled, QPalette::ButtonText, colors.mutedText);
        palette.setColor(QPalette::Disabled, QPalette::Base, colors.raisedSurface);
        return palette;
    }

    [[nodiscard]] QString createStyleSheet(const ThemeColors& colors)
    {
        QString styleSheet = QStringLiteral(R"(
QMainWindow, QDialog {
    background-color: @WINDOW@;
    color: @TEXT@;
}
QLabel {
    color: @TEXT@;
    background-color: transparent;
}
QLabel#pageTitleLabel {
    font-size: 18px;
    font-weight: 600;
    padding: 10px;
}
QLabel#helpLabel,
QLabel#monthlyStatisticsLabel,
QLabel#calculationResultLabel {
    background-color: @SURFACE@;
    border: 1px solid @BORDER@;
    border-radius: 4px;
    padding: 10px;
}
QLabel#helpLabel {
    color: @MUTED_TEXT@;
    font-size: 12px;
}
QLabel#settingsErrorLabel {
    color: @ERROR@;
    padding: 4px;
}
QGroupBox {
    color: @TEXT@;
    background-color: transparent;
    border: 1px solid @BORDER@;
    border-radius: 4px;
    margin-top: 8px;
    padding-top: 8px;
}
QGroupBox::title {
    subcontrol-origin: margin;
    left: 8px;
    padding: 0 4px;
    color: @TEXT@;
    background-color: @WINDOW@;
}
QPushButton {
    color: @TEXT@;
    background-color: @RAISED_SURFACE@;
    border: 1px solid @BORDER@;
    border-radius: 4px;
    padding: 5px 10px;
}
QPushButton:hover {
    background-color: @SURFACE@;
    border-color: @ACCENT_HOVER@;
}
QPushButton:pressed {
    background-color: @BORDER@;
}
QPushButton#collapsibleToggleButton {
    background-color: transparent;
    border: none;
    font-weight: 600;
    padding: 6px 4px;
    text-align: left;
}
QPushButton#collapsibleToggleButton:hover,
QPushButton#collapsibleToggleButton:checked {
    background-color: @RAISED_SURFACE@;
}
QTimeEdit, QDoubleSpinBox {
    color: @TEXT@;
    background-color: @SURFACE@;
    border: 1px solid @BORDER@;
    border-radius: 3px;
    padding: 4px 6px;
    selection-background-color: @ACCENT@;
    selection-color: @ACCENT_TEXT@;
}
QTimeEdit:focus, QDoubleSpinBox:focus {
    border-color: @ACCENT@;
}
QMenu {
    color: @TEXT@;
    background-color: @SURFACE@;
    border: 1px solid @BORDER@;
}
QMenu::item:selected {
    color: @ACCENT_TEXT@;
    background-color: @ACCENT@;
}
QToolTip {
    color: @TEXT@;
    background-color: @RAISED_SURFACE@;
    border: 1px solid @BORDER@;
}
QCalendarWidget {
    color: @TEXT@;
    background-color: @SURFACE@;
    border: 1px solid @BORDER@;
}
QCalendarWidget QWidget#qt_calendar_navigationbar {
    background-color: @ACCENT@;
}
QCalendarWidget QToolButton {
    color: @ACCENT_TEXT@;
    background-color: transparent;
    border: none;
    border-radius: 0;
    font-weight: 600;
    padding: 6px;
}
QCalendarWidget QToolButton:hover {
    background-color: @ACCENT_HOVER@;
}
QCalendarWidget QToolButton:pressed {
    background-color: @ACCENT_PRESSED@;
}
QCalendarWidget QTableView {
    background-color: @SURFACE@;
    alternate-background-color: @SURFACE@;
    gridline-color: @BORDER@;
    selection-background-color: @ACCENT@;
    selection-color: @ACCENT_TEXT@;
    outline: none;
}
QCalendarWidget QHeaderView::section {
    color: @TEXT@;
    background-color: @RAISED_SURFACE@;
    border: none;
    border-right: 1px solid @BORDER@;
    border-bottom: 1px solid @BORDER@;
    padding: 8px 4px;
}
QSplitter::handle {
    background-color: transparent;
}
)");

        const std::pair<QString, QColor> replacements[] = {
            { QStringLiteral("@WINDOW@"), colors.window },
            { QStringLiteral("@SURFACE@"), colors.surface },
            { QStringLiteral("@RAISED_SURFACE@"), colors.raisedSurface },
            { QStringLiteral("@BORDER@"), colors.border },
            { QStringLiteral("@TEXT@"), colors.text },
            { QStringLiteral("@MUTED_TEXT@"), colors.mutedText },
            { QStringLiteral("@ACCENT@"), colors.accent },
            { QStringLiteral("@ACCENT_HOVER@"), colors.accentHover },
            { QStringLiteral("@ACCENT_PRESSED@"), colors.accentPressed },
            { QStringLiteral("@ACCENT_TEXT@"), colors.accentText },
            { QStringLiteral("@ERROR@"), colors.error },
        };
        for (const auto& [token, color] : replacements) {
            styleSheet.replace(token, color.name());
        }
        return styleSheet;
    }
} // namespace

void AttendanceTheme::apply(QApplication& application, const Qt::ColorScheme colorScheme)
{
    const ThemeColors colors = colorsFor(isDarkTheme(application, colorScheme));
    application.setPalette(createPalette(colors));
    application.setStyleSheet(createStyleSheet(colors));
}

QColor AttendanceTheme::attendanceBackground(const QPalette& palette, const bool excludedFromAverage)
{
    const bool dark = palette.color(QPalette::Window).lightness() < 128;
    if (dark) {
        return excludedFromAverage ? QColor("#214b48") : QColor("#244d43");
    }
    return excludedFromAverage ? QColor("#ddf4f0") : QColor("#d9f2e7");
}

QColor AttendanceTheme::attendanceForeground(const QPalette& palette)
{
    return palette.color(QPalette::Window).lightness() < 128 ? QColor("#e4fbf4") : QColor("#174c3c");
}

QColor AttendanceTheme::weekendForeground(const QPalette& palette)
{
    return palette.color(QPalette::Window).lightness() < 128 ? QColor("#ff968f") : QColor("#9c3038");
}
