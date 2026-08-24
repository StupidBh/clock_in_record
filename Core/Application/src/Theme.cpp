#include "Application/Theme.h"

#include <QApplication>
#include <QPalette>

#include <array>
#include <utility>

using namespace Qt::StringLiterals;

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
        QColor success;
        QColor warning;
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
                QColor(u"#1b1d20"_s), QColor(u"#25282c"_s), QColor(u"#30343a"_s), QColor(u"#464b52"_s),
                QColor(u"#f2f4f7"_s), QColor(u"#aeb4bc"_s), QColor(u"#176da8"_s), QColor(u"#2082c2"_s),
                QColor(u"#11557f"_s), QColor(u"#ffffff"_s), QColor(u"#ff7b86"_s), QColor(u"#72d6aa"_s),
                QColor(u"#f2c66d"_s), QColor(u"#72bff0"_s),
            };
        }

        return {
            QColor(u"#f4f6f8"_s), QColor(u"#ffffff"_s), QColor(u"#e9eef2"_s), QColor(u"#c8d0d8"_s),
            QColor(u"#20242a"_s), QColor(u"#5f6872"_s), QColor(u"#0b639b"_s), QColor(u"#0e78b8"_s),
            QColor(u"#084d79"_s), QColor(u"#ffffff"_s), QColor(u"#b4232d"_s), QColor(u"#15724d"_s),
            QColor(u"#875b08"_s), QColor(u"#075f96"_s),
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
    font-size: 20px;
    font-weight: 600;
    padding: 0;
}
QLabel#sectionTitleLabel {
    font-size: 14px;
    font-weight: 600;
}
QLabel#statisticsPeriodLabel,
QLabel#dialogDateLabel,
QLabel#calculationResultDetailLabel,
QLabel#settingsStatusLabel,
QLabel[statisticsRole="metric"],
QLabel[resultRole="metric"] {
    color: @MUTED_TEXT@;
}
QLabel[statisticsRole="value"],
QLabel[resultRole="value"] {
    font-weight: 600;
}
QLabel#statsTargetValueLabel[tone="warning"],
QLabel#calculationResultLabel[tone="warning"] {
    color: @WARNING@;
}
QLabel#statsTargetValueLabel[tone="positive"],
QLabel#calculationResultLabel[tone="positive"] {
    color: @SUCCESS@;
}
QLabel#calculationResultLabel {
    font-size: 15px;
    font-weight: 600;
}
QLabel#formSectionLabel {
    color: @TEXT@;
    font-weight: 600;
    padding-top: 2px;
}
QWidget#calculationResultPanel {
    background-color: @SURFACE@;
    border: 1px solid @BORDER@;
    border-radius: 4px;
}
QLabel#settingsErrorLabel {
    color: @ERROR@;
    padding: 3px 0;
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
QWidget#inspectorPanel {
    background-color: @SURFACE@;
}
QScrollArea#inspectorScrollArea {
    background-color: @SURFACE@;
    border-left: 1px solid @BORDER@;
}
QScrollArea#globalSettingsScrollArea,
QScrollArea#globalSettingsScrollArea > QWidget > QWidget {
    background-color: transparent;
}
QFrame#inspectorSeparator {
    color: @BORDER@;
}
QPushButton {
    color: @TEXT@;
    background-color: @RAISED_SURFACE@;
    border: 1px solid @BORDER@;
    border-radius: 4px;
    min-height: 22px;
    padding: 5px 11px;
}
QPushButton:hover {
    background-color: @SURFACE@;
    border-color: @ACCENT_HOVER@;
}
QPushButton:pressed {
    background-color: @BORDER@;
}
QPushButton#primaryButton {
    color: @ACCENT_TEXT@;
    background-color: @ACCENT@;
    border-color: @ACCENT@;
}
QPushButton#primaryButton:hover {
    background-color: @ACCENT_HOVER@;
    border-color: @ACCENT_HOVER@;
}
QPushButton#primaryButton:pressed {
    background-color: @ACCENT_PRESSED@;
    border-color: @ACCENT_PRESSED@;
}
QPushButton#deleteRecordButton {
    color: @ERROR@;
    background-color: transparent;
}
QPushButton#deleteRecordButton:hover {
    border-color: @ERROR@;
}
QToolButton#collapsibleToggleButton {
    color: @TEXT@;
    background-color: transparent;
    border: none;
    font-weight: 600;
    padding: 6px 4px;
    text-align: left;
}
QToolButton#collapsibleToggleButton:hover,
QToolButton#collapsibleToggleButton:checked {
    background-color: @RAISED_SURFACE@;
}
QTimeEdit, QDoubleSpinBox {
    color: @TEXT@;
    background-color: @SURFACE@;
    border: 1px solid @BORDER@;
    border-radius: 3px;
    min-height: 24px;
    padding: 3px 6px;
    selection-background-color: @ACCENT@;
    selection-color: @ACCENT_TEXT@;
}
QTimeEdit:focus, QDoubleSpinBox:focus {
    border-color: @ACCENT@;
}
QTimeEdit[validationError="true"] {
    border-color: @ERROR@;
}
QTimeEdit::up-button, QDoubleSpinBox::up-button {
    subcontrol-origin: border;
    subcontrol-position: top right;
    width: 18px;
    background-color: @RAISED_SURFACE@;
    border-left: 1px solid @BORDER@;
    border-bottom: 1px solid @BORDER@;
    border-top-right-radius: 3px;
}
QTimeEdit::down-button, QDoubleSpinBox::down-button {
    subcontrol-origin: border;
    subcontrol-position: bottom right;
    width: 18px;
    background-color: @RAISED_SURFACE@;
    border-left: 1px solid @BORDER@;
    border-bottom-right-radius: 3px;
}
QTimeEdit::up-button:hover, QTimeEdit::down-button:hover,
QDoubleSpinBox::up-button:hover, QDoubleSpinBox::down-button:hover {
    background-color: @BORDER@;
}
QTimeEdit::up-button:pressed, QTimeEdit::down-button:pressed,
QDoubleSpinBox::up-button:pressed, QDoubleSpinBox::down-button:pressed {
    background-color: @SURFACE@;
}
QTimeEdit::up-arrow, QDoubleSpinBox::up-arrow {
    image: url(@SPIN_UP_ARROW@);
    width: 8px;
    height: 5px;
}
QTimeEdit::down-arrow, QDoubleSpinBox::down-arrow {
    image: url(@SPIN_DOWN_ARROW@);
    width: 8px;
    height: 5px;
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
    background-color: @RAISED_SURFACE@;
    border-bottom: 1px solid @BORDER@;
}
QCalendarWidget QToolButton {
    color: @TEXT@;
    background-color: transparent;
    border: none;
    border-radius: 0;
    font-weight: 600;
    padding: 6px;
}
QCalendarWidget QToolButton:hover {
    background-color: @SURFACE@;
}
QCalendarWidget QToolButton:pressed {
    background-color: @BORDER@;
}
QCalendarWidget QToolButton#qt_calendar_prevmonth,
QCalendarWidget QToolButton#qt_calendar_nextmonth {
    min-width: 34px;
    font-size: 18px;
    font-weight: 400;
    padding: 2px 6px;
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
    width: 8px;
}
)");

        const std::array replacements {
            std::pair { u"@WINDOW@"_s, colors.window },
            std::pair { u"@SURFACE@"_s, colors.surface },
            std::pair { u"@RAISED_SURFACE@"_s, colors.raisedSurface },
            std::pair { u"@BORDER@"_s, colors.border },
            std::pair { u"@TEXT@"_s, colors.text },
            std::pair { u"@MUTED_TEXT@"_s, colors.mutedText },
            std::pair { u"@ACCENT@"_s, colors.accent },
            std::pair { u"@ACCENT_HOVER@"_s, colors.accentHover },
            std::pair { u"@ACCENT_PRESSED@"_s, colors.accentPressed },
            std::pair { u"@ACCENT_TEXT@"_s, colors.accentText },
            std::pair { u"@ERROR@"_s, colors.error },
            std::pair { u"@SUCCESS@"_s, colors.success },
            std::pair { u"@WARNING@"_s, colors.warning },
        };
        for (const auto& [token, color] : replacements) {
            styleSheet.replace(token, color.name());
        }
        const bool dark = colors.window.lightness() < 128;
        styleSheet.replace(u"@SPIN_UP_ARROW@"_s,
                           dark ? u":/Icons/spin-arrow-up-dark.svg"_s : u":/Icons/spin-arrow-up-light.svg"_s);
        styleSheet.replace(u"@SPIN_DOWN_ARROW@"_s,
                           dark ? u":/Icons/spin-arrow-down-dark.svg"_s : u":/Icons/spin-arrow-down-light.svg"_s);
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
        return excludedFromAverage ? QColor(u"#38424a"_s) : QColor(u"#244d43"_s);
    }
    return excludedFromAverage ? QColor(u"#e5eaef"_s) : QColor(u"#d9f2e7"_s);
}

QColor AttendanceTheme::attendanceForeground(const QPalette& palette)
{
    return palette.color(QPalette::Window).lightness() < 128 ? QColor(u"#e4fbf4"_s) : QColor(u"#174c3c"_s);
}

QColor AttendanceTheme::weekendForeground(const QPalette& palette)
{
    return palette.color(QPalette::Window).lightness() < 128 ? QColor(u"#ff968f"_s) : QColor(u"#9c3038"_s);
}
