#include "Application/AttendanceMainWindow.h"
#include "Calendar/CustomCalendarWidget.h"
#include "Settings/AttendanceSettings.h"
#include "Settings/TimeSettingDialog.h"

#include <QAbstractButton>
#include <QApplication>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSettings>
#include <QTemporaryDir>
#include <QTimer>
#include <QToolButton>

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

    void expectFalse(const char* name, const bool value)
    {
        expectTrue(name, !value);
    }

    void testMainWindowStructure()
    {
        AttendanceMainWindow window;

        auto* const calendar = window.findChild<CustomCalendarWidget*>();
        expectTrue("calendar exists", calendar != nullptr);
        if (calendar) {
            expectTrue("calendar week numbers hidden",
                       calendar->verticalHeaderFormat() == QCalendarWidget::NoVerticalHeader);
        }

        auto* const inspector = window.findChild<QScrollArea*>(u"inspectorScrollArea"_s);
        expectTrue("scrollable inspector exists", inspector != nullptr);
        if (inspector) {
            expectTrue("inspector resizes content", inspector->widgetResizable());
            expectTrue("inspector avoids horizontal scrolling",
                       inspector->horizontalScrollBarPolicy() == Qt::ScrollBarAlwaysOff);
        }

        auto* const statisticsLabel = window.findChild<QLabel*>(u"monthlyStatisticsLabel"_s);
        expectTrue("statistics label exists", statisticsLabel != nullptr);
        if (statisticsLabel) {
            expectTrue("statistics avoids nested group box",
                       qobject_cast<QGroupBox*>(statisticsLabel->parentWidget()) == nullptr);
        }

        auto* const settingsToggle = window.findChild<QToolButton*>(u"collapsibleToggleButton"_s);
        expectTrue("settings toggle exists", settingsToggle != nullptr);
        if (settingsToggle) {
            expectTrue("settings initially collapsed", settingsToggle->arrowType() == Qt::RightArrow);
            settingsToggle->setChecked(true);
            expectTrue("settings expands with standard arrow", settingsToggle->arrowType() == Qt::DownArrow);
        }
    }

    void testRecordActions()
    {
        QSettings settings;
        settings.clear();

        const QDate date(2026, 8, 21);
        TimeSettingDialog newRecordDialog(date);
        auto* const newRecordDeleteButton = newRecordDialog.findChild<QPushButton*>(u"deleteRecordButton"_s);
        expectTrue("new record delete button exists", newRecordDeleteButton != nullptr);
        if (newRecordDeleteButton) {
            expectTrue("new record delete button hidden", newRecordDeleteButton->isHidden());
        }

        auto* const saveButton = newRecordDialog.findChild<QPushButton*>(u"primaryButton"_s);
        expectTrue("save button exists", saveButton != nullptr);
        if (saveButton) {
            saveButton->click();
            expectTrue("dialog save stores record", AttendanceSettings::hasRecord(settings, date));
        }

        TimeSettingDialog existingRecordDialog(date);
        auto* const existingRecordDeleteButton = existingRecordDialog.findChild<QPushButton*>(u"deleteRecordButton"_s);
        expectTrue("existing record delete button exists", existingRecordDeleteButton != nullptr);
        if (existingRecordDeleteButton) {
            expectTrue("existing record delete button shown", !existingRecordDeleteButton->isHidden());
        }

        auto* const resultLabel = existingRecordDialog.findChild<QLabel*>(u"calculationResultLabel"_s);
        expectTrue("calculation result label exists", resultLabel != nullptr);
        if (resultLabel) {
            expectTrue("calculation result can grow", resultLabel->maximumHeight() > resultLabel->minimumHeight());
        }

        if (existingRecordDeleteButton) {
            existingRecordDialog.show();
            QApplication::processEvents();
            QTimer::singleShot(50, []() {
                for (QWidget* const widget : QApplication::topLevelWidgets()) {
                    if (auto* const messageBox = qobject_cast<QMessageBox*>(widget)) {
                        if (QAbstractButton* const yesButton = messageBox->button(QMessageBox::Yes)) {
                            yesButton->click();
                        }
                    }
                }
            });
            existingRecordDeleteButton->click();
            expectFalse("dialog delete removes record", AttendanceSettings::hasRecord(settings, date));
        }

        settings.clear();
    }
} // namespace

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);
    QTemporaryDir settingsDirectory;
    if (!settingsDirectory.isValid()) {
        std::cerr << "failed to create temporary settings directory\n";
        return 1;
    }

    QCoreApplication::setOrganizationName(u"AttendanceAppTests"_s);
    QCoreApplication::setApplicationName(u"ApplicationUiTests"_s);
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDirectory.path());

    testMainWindowStructure();
    testRecordActions();

    return failures == 0 ? 0 : 1;
}
