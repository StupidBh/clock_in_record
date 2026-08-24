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
#include <QScrollBar>
#include <QSettings>
#include <QSizePolicy>
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
            expectTrue("inspector scrolls vertically only as a fallback",
                       inspector->verticalScrollBarPolicy() == Qt::ScrollBarAsNeeded);
        }

        auto* const settingsScrollArea = window.findChild<QScrollArea*>(u"globalSettingsScrollArea"_s);
        expectTrue("global settings scroll area exists", settingsScrollArea != nullptr);
        if (settingsScrollArea) {
            expectTrue("global settings resizes content", settingsScrollArea->widgetResizable());
            expectTrue("global settings avoids horizontal scrolling",
                       settingsScrollArea->horizontalScrollBarPolicy() == Qt::ScrollBarAlwaysOff);
            expectTrue("global settings scrolls vertically as needed",
                       settingsScrollArea->verticalScrollBarPolicy() == Qt::ScrollBarAsNeeded);
        }

        auto* const globalSettingsGroup = window.findChild<QWidget*>(u"globalSettingsGroup"_s);
        auto* const inspectorSeparator = window.findChild<QWidget*>(u"inspectorSeparator"_s);
        expectTrue("global settings group exists", globalSettingsGroup != nullptr);
        if (globalSettingsGroup && settingsScrollArea) {
            expectTrue("settings scroll area stays inside global settings",
                       globalSettingsGroup->isAncestorOf(settingsScrollArea));
            expectTrue("collapsed settings use fixed height",
                       globalSettingsGroup->sizePolicy().verticalPolicy() == QSizePolicy::Fixed);
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
            window.resize(window.minimumSize());
            window.show();
            QApplication::processEvents();
            if (globalSettingsGroup && inspectorSeparator) {
                const int distanceFromSeparator = globalSettingsGroup->y() - inspectorSeparator->geometry().bottom();
                expectTrue("collapsed settings stay directly below statistics",
                           distanceFromSeparator > 0 && distanceFromSeparator <= 24);
            }
            const int collapsedToggleY = settingsToggle->mapTo(&window, QPoint()).y();
            settingsToggle->setChecked(true);
            QApplication::processEvents();
            expectTrue("settings expands with standard arrow", settingsToggle->arrowType() == Qt::DownArrow);
            expectTrue("expanded settings use remaining height",
                       globalSettingsGroup &&
                           globalSettingsGroup->sizePolicy().verticalPolicy() == QSizePolicy::Expanding);
            expectTrue("settings header stays in place when expanded",
                       settingsToggle->mapTo(&window, QPoint()).y() == collapsedToggleY);
            if (inspector && settingsScrollArea) {
                expectTrue("normal window keeps inspector fixed", inspector->verticalScrollBar()->maximum() == 0);
                expectTrue("expanded settings scroll internally",
                           settingsScrollArea->verticalScrollBar()->maximum() > 0);

                window.setMinimumSize(300, 300);
                window.resize(720, 300);
                QApplication::processEvents();
                expectTrue("short window enables inspector fallback scrolling",
                           inspector->verticalScrollBar()->maximum() > 0);
            }
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
