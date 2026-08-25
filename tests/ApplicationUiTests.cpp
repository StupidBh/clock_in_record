#include "Application/AttendanceMainWindow.h"
#include "Calendar/CustomCalendarWidget.h"
#include "Settings/AttendanceSettings.h"
#include "Settings/TimeSettingDialog.h"

#include <QAbstractButton>
#include <QAction>
#include <QApplication>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QMenu>
#include <QMouseEvent>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSettings>
#include <QSizePolicy>
#include <QTemporaryDir>
#include <QTextCharFormat>
#include <QTimer>
#include <QToolButton>

#include <iostream>

using namespace Qt::StringLiterals;

namespace {
    int failures = 0;

    void clickMenuAction(QMenu& menu, QAction& action)
    {
        const QPoint localPosition = menu.actionGeometry(&action).center();
        const QPoint globalPosition = menu.mapToGlobal(localPosition);
        QMouseEvent moveEvent(QEvent::MouseMove,
                              localPosition,
                              globalPosition,
                              Qt::NoButton,
                              Qt::NoButton,
                              Qt::NoModifier);
        QApplication::sendEvent(&menu, &moveEvent);
        QMouseEvent pressEvent(QEvent::MouseButtonPress,
                               localPosition,
                               globalPosition,
                               Qt::LeftButton,
                               Qt::LeftButton,
                               Qt::NoModifier);
        QApplication::sendEvent(&menu, &pressEvent);
        QMouseEvent releaseEvent(QEvent::MouseButtonRelease,
                                 localPosition,
                                 globalPosition,
                                 Qt::LeftButton,
                                 Qt::NoButton,
                                 Qt::NoModifier);
        QApplication::sendEvent(&menu, &releaseEvent);
        action.trigger();
        menu.close();
    }

    bool clickWithoutUnexpectedMessageBox(QAbstractButton& button)
    {
        bool messageBoxShown = false;
        QTimer messageBoxGuard;
        messageBoxGuard.setSingleShot(true);
        QObject::connect(&messageBoxGuard, &QTimer::timeout, &button, [&messageBoxShown]() {
            for (QWidget* const widget : QApplication::topLevelWidgets()) {
                if (auto* const messageBox = qobject_cast<QMessageBox*>(widget)) {
                    messageBoxShown = true;
                    messageBox->reject();
                }
            }
        });

        messageBoxGuard.start(100);
        button.click();
        messageBoxGuard.stop();
        return !messageBoxShown;
    }

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

            QApplication::processEvents();
            auto* const monthButton = calendar->findChild<QToolButton*>(u"qt_calendar_monthbutton"_s);
            auto* const statisticsPeriod = window.findChild<QLabel*>(u"statisticsPeriodLabel"_s);
            expectTrue("calendar month menu exists", monthButton && monthButton->menu());
            expectTrue("statistics period exists", statisticsPeriod != nullptr);
            if (monthButton && monthButton->menu() && statisticsPeriod) {
                const int targetMonth = today.addMonths(2).month();
                const int previousMonthNumber = today.addMonths(-1).month();
                QAction* originalMonthAction = nullptr;
                QAction* previousMonthAction = nullptr;
                QAction* targetMonthAction = nullptr;
                for (QAction* const action : monthButton->menu()->actions()) {
                    const int month = action->data().toInt();
                    if (month == today.month()) {
                        originalMonthAction = action;
                    }
                    else if (month == previousMonthNumber) {
                        previousMonthAction = action;
                    }
                    else if (month == targetMonth) {
                        targetMonthAction = action;
                    }
                }

                expectTrue("original month action exists", originalMonthAction != nullptr);
                expectTrue("previous month action exists", previousMonthAction != nullptr);
                expectTrue("target month action exists", targetMonthAction != nullptr);
                if (originalMonthAction && previousMonthAction && targetMonthAction) {
                    window.show();
                    QApplication::processEvents();
                    const auto selectMonthFromMenu = [monthButton](QAction* const action) {
                        QTimer::singleShot(10, monthButton->menu(), [menu = monthButton->menu(), action]() {
                            menu->setActiveAction(action);
                            clickMenuAction(*menu, *action);
                        });
                        monthButton->showMenu();
                        QApplication::processEvents();
                    };

                    selectMonthFromMenu(targetMonthAction);
                    expectTrue("month popup selects another month", calendar->monthShown() == targetMonth);
                    expectTrue("month popup preserves date selection",
                               calendar->selectionMode() == QCalendarWidget::SingleSelection);
                    selectMonthFromMenu(previousMonthAction);
                    expectTrue("month popup selects previous month", calendar->monthShown() == previousMonthNumber);
                    expectTrue("reopened month popup preserves date selection",
                               calendar->selectionMode() == QCalendarWidget::SingleSelection);
                    selectMonthFromMenu(originalMonthAction);
                    expectTrue("month popup returns to current month", calendar->monthShown() == today.month());
                    expectTrue("current month popup preserves date selection",
                               calendar->selectionMode() == QCalendarWidget::SingleSelection);

                    const QString originalPeriod = statisticsPeriod->text();
                    targetMonthAction->trigger();
                    expectTrue("month menu changes page synchronously", calendar->monthShown() == targetMonth);
                    expectTrue("month menu defers statistics refresh", statisticsPeriod->text() == originalPeriod);

                    QApplication::processEvents();
                    expectTrue("deferred month refresh uses selected year", calendar->yearShown() == today.year());
                    expectTrue("deferred month refresh uses selected month", calendar->monthShown() == targetMonth);
                    expectTrue("deferred month refresh updates statistics period",
                               statisticsPeriod->text() == u"%1年%2月"_s.arg(today.year()).arg(targetMonth));

                    for (int iteration = 0; iteration < 101; ++iteration) {
                        (iteration % 2 == 0 ? originalMonthAction : targetMonthAction)->trigger();
                    }
                    expectTrue("rapid month menu changes end on latest page", calendar->monthShown() == today.month());
                    expectTrue("rapid month menu changes keep refresh deferred",
                               statisticsPeriod->text() == u"%1年%2月"_s.arg(today.year()).arg(targetMonth));

                    QApplication::processEvents();
                    expectTrue("coalesced month refresh uses latest page",
                               statisticsPeriod->text() == u"%1年%2月"_s.arg(today.year()).arg(today.month()));

                    const QDate maximumDate = calendar->maximumDate();
                    const bool invoked = QMetaObject::invokeMethod(&window,
                                                                   "onMonthChanged",
                                                                   Qt::DirectConnection,
                                                                   Q_ARG(int, maximumDate.year() + 1),
                                                                   Q_ARG(int, maximumDate.month()));
                    expectTrue("out-of-range month change is handled", invoked);
                    expectTrue("out-of-range year is clamped to maximum year",
                               calendar->yearShown() == maximumDate.year());
                    expectTrue("out-of-range year is clamped to maximum month",
                               calendar->monthShown() == maximumDate.month());

                    QApplication::processEvents();
                    expectTrue("clamped month refreshes statistics",
                               statisticsPeriod->text() ==
                                   u"%1年%2月"_s.arg(maximumDate.year()).arg(maximumDate.month()));
                }
            }
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
            expectTrue("dialog save avoids unexpected message box", clickWithoutUnexpectedMessageBox(*saveButton));
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
            bool unexpectedDeleteMessage = false;
            QTimer deleteMessageGuard;
            QObject::connect(&deleteMessageGuard,
                             &QTimer::timeout,
                             existingRecordDeleteButton,
                             [&unexpectedDeleteMessage]() {
                                 for (QWidget* const widget : QApplication::topLevelWidgets()) {
                                     if (auto* const messageBox = qobject_cast<QMessageBox*>(widget)) {
                                         if (QAbstractButton* const yesButton = messageBox->button(QMessageBox::Yes)) {
                                             yesButton->click();
                                         }
                                         else {
                                             unexpectedDeleteMessage = true;
                                             messageBox->reject();
                                         }
                                     }
                                 }
                             });
            deleteMessageGuard.start(50);
            existingRecordDeleteButton->click();
            deleteMessageGuard.stop();
            expectFalse("dialog delete avoids unexpected message box", unexpectedDeleteMessage);
            expectFalse("dialog delete removes record", AttendanceSettings::hasRecord(settings, date));
        }

        settings.clear();
    }

    void testCalendarPresentationSnapshot()
    {
        QSettings settings;
        settings.clear();

        CustomCalendarWidget calendarProbe;
        const QDate currentMonth(QDate::currentDate().year(), QDate::currentDate().month(), 1);
        const int leadingDays = (currentMonth.dayOfWeek() - static_cast<int>(calendarProbe.firstDayOfWeek()) + 7) % 7;
        const QDate adjacentDate =
            leadingDays > 0 ? currentMonth.addDays(-1) : currentMonth.addDays(currentMonth.daysInMonth());
        const QDate distantDate = currentMonth.addMonths(3);
        const QDate maximumDate = calendarProbe.maximumDate();

        const AttendanceRecord schedule;
        for (const QDate date : { currentMonth, adjacentDate, distantDate, maximumDate }) {
            AttendanceRecord record = AttendanceSettings::createRecord(date, schedule);
            record.arrivalTime = QTime(8, 30);
            record.departureTime = QTime(18, 30);
            expectTrue("calendar snapshot record save", AttendanceSettings::saveRecord(settings, date, record));
        }

        {
            AttendanceMainWindow window;
            auto* const calendar = window.findChild<CustomCalendarWidget*>();
            expectTrue("calendar snapshot widget exists", calendar != nullptr);
            if (calendar) {
                expectFalse("current month record has a date format", calendar->dateTextFormat(currentMonth).isEmpty());
                expectFalse("visible adjacent record has a date format",
                            calendar->dateTextFormat(adjacentDate).isEmpty());

                calendar->setCurrentPage(distantDate.year(), distantDate.month());
                QApplication::processEvents();

                expectTrue("old month date format is cleared", calendar->dateTextFormat(currentMonth).isEmpty());
                expectFalse("new month record has a date format", calendar->dateTextFormat(distantDate).isEmpty());

                calendar->setCurrentPage(maximumDate.year(), maximumDate.month());
                QApplication::processEvents();

                expectTrue("previous month date format is cleared", calendar->dateTextFormat(distantDate).isEmpty());
                expectFalse("maximum date record has a date format", calendar->dateTextFormat(maximumDate).isEmpty());
            }
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
    testCalendarPresentationSnapshot();
    testRecordActions();

    return failures == 0 ? 0 : 1;
}
