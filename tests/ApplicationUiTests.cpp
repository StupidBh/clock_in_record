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

            auto* const previousMonthButton = calendar->findChild<QToolButton*>(u"qt_calendar_prevmonth"_s);
            auto* const nextMonthButton = calendar->findChild<QToolButton*>(u"qt_calendar_nextmonth"_s);
            expectTrue("calendar previous month button uses clear text arrow",
                       previousMonthButton && previousMonthButton->text() == u"‹"_s);
            expectTrue("calendar next month button uses clear text arrow",
                       nextMonthButton && nextMonthButton->text() == u"›"_s);
        }

        auto* const inspector = window.findChild<QScrollArea*>(u"inspectorScrollArea"_s);
        expectTrue("scrollable inspector exists", inspector != nullptr);
        if (inspector) {
            expectTrue("inspector resizes content", inspector->widgetResizable());
            expectTrue("inspector avoids horizontal scrolling",
                       inspector->horizontalScrollBarPolicy() == Qt::ScrollBarAlwaysOff);
        }

        auto* const statisticsValue = window.findChild<QLabel*>(u"statsWorkDaysValueLabel"_s);
        expectTrue("structured statistics exist", statisticsValue != nullptr);
        if (statisticsValue) {
            expectTrue("statistics avoid nested group box",
                       qobject_cast<QGroupBox*>(statisticsValue->parentWidget()) == nullptr);
        }

        auto* const todayButton = window.findChild<QPushButton*>(u"todayButton"_s);
        expectTrue("today button exists", todayButton != nullptr);
        if (calendar && todayButton) {
            const QDate today = QDate::currentDate();
            const QDate previousMonth = today.addMonths(-1);
            calendar->setCurrentPage(previousMonth.year(), previousMonth.month());
            calendar->clearDateSelection();
            todayButton->click();
            expectTrue("today button returns to current year", calendar->yearShown() == today.year());
            expectTrue("today button returns to current month", calendar->monthShown() == today.month());
            expectTrue("today button restores date selection",
                       calendar->selectionMode() == QCalendarWidget::SingleSelection);
            expectTrue("today button selects today", calendar->selectedDate() == today);
        }

        auto* const settingsStatus = window.findChild<QLabel*>(u"settingsStatusLabel"_s);
        expectTrue("settings persistence status exists", settingsStatus != nullptr);

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
            expectTrue("save button avoids legacy icon", saveButton->icon().isNull());
            saveButton->click();
            expectTrue("dialog save stores record", AttendanceSettings::hasRecord(settings, date));
        }

        TimeSettingDialog existingRecordDialog(date);
        auto* const existingRecordDeleteButton = existingRecordDialog.findChild<QPushButton*>(u"deleteRecordButton"_s);
        expectTrue("existing record delete button exists", existingRecordDeleteButton != nullptr);
        if (existingRecordDeleteButton) {
            expectTrue("existing record delete button shown", !existingRecordDeleteButton->isHidden());
            expectTrue("delete button avoids legacy icon", existingRecordDeleteButton->icon().isNull());
        }

        auto* const resultLabel = existingRecordDialog.findChild<QLabel*>(u"calculationResultLabel"_s);
        expectTrue("calculation result label exists", resultLabel != nullptr);
        if (resultLabel) {
            expectFalse("calculation result avoids log syntax", resultLabel->text().contains(u"["_s));
        }

        expectTrue("structured actual work metric exists",
                   existingRecordDialog.findChild<QLabel*>(u"actualWorkValueLabel"_s) != nullptr);

        auto* const cancelButton = existingRecordDialog.findChild<QPushButton*>(u"cancelButton"_s);
        auto* const existingSaveButton = existingRecordDialog.findChild<QPushButton*>(u"primaryButton"_s);
        expectTrue("cancel button exists", cancelButton != nullptr);
        if (cancelButton) {
            expectTrue("cancel button avoids legacy icon", cancelButton->icon().isNull());
        }

        if (existingRecordDeleteButton) {
            existingRecordDialog.show();
            QApplication::processEvents();
            if (cancelButton && existingSaveButton) {
                expectTrue("delete action stays left of cancel", existingRecordDeleteButton->x() < cancelButton->x());
                expectTrue("save action stays rightmost", cancelButton->x() < existingSaveButton->x());
            }
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
