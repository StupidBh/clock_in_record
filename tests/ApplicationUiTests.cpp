#include "Application/AttendanceMainWindow.h"
#include "Calendar/CustomCalendarWidget.h"
#include "Settings/AttendanceSettings.h"
#include "Settings/TimeSettingDialog.h"

#include <QAbstractButton>
#include <QAction>
#include <QApplication>
#include <QDoubleSpinBox>
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
#include <QTimeEdit>
#include <QTimer>
#include <QToolButton>
#include <QWheelEvent>

#include <array>
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

    void sendWheelEvent(QWidget& widget, const int angleDeltaY)
    {
        const QPointF localPosition = widget.rect().center();
        QWheelEvent event(localPosition,
                          widget.mapToGlobal(localPosition.toPoint()),
                          QPoint(),
                          QPoint(0, angleDeltaY),
                          Qt::NoButton,
                          Qt::NoModifier,
                          Qt::NoScrollPhase,
                          false);
        QApplication::sendEvent(&widget, &event);
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

                auto* const workStartEdit = window.findChild<QTimeEdit*>(u"globalWorkStartEdit"_s);
                auto* const overtimeTarget = window.findChild<QDoubleSpinBox*>(u"targetOvertimeHoursSpinBox"_s);
                expectTrue("global time editor exists", workStartEdit != nullptr);
                expectTrue("overtime target editor exists", overtimeTarget != nullptr);
                if (workStartEdit && overtimeTarget) {
                    expectTrue("time editor cannot gain focus from wheel",
                               workStartEdit->focusPolicy() == Qt::StrongFocus);
                    expectTrue("overtime editor cannot gain focus from wheel",
                               overtimeTarget->focusPolicy() == Qt::StrongFocus);
                    const QTime originalWorkStart = workStartEdit->time();
                    const double originalOvertimeTarget = overtimeTarget->value();
                    workStartEdit->clearFocus();
                    overtimeTarget->clearFocus();
                    settingsScrollArea->verticalScrollBar()->setValue(0);

                    sendWheelEvent(*workStartEdit, -120);
                    expectTrue("unfocused time editor ignores wheel", workStartEdit->time() == originalWorkStart);
                    expectTrue("unfocused editor passes wheel to settings scroll area",
                               settingsScrollArea->verticalScrollBar()->value() > 0);

                    sendWheelEvent(*overtimeTarget, 120);
                    expectTrue("unfocused overtime editor ignores wheel",
                               overtimeTarget->value() == originalOvertimeTarget);

                    workStartEdit->setFocus();
                    workStartEdit->setCurrentSection(QDateTimeEdit::MinuteSection);
                    sendWheelEvent(*workStartEdit, 120);
                    expectTrue("focused time editor accepts wheel", workStartEdit->time() != originalWorkStart);
                    workStartEdit->setTime(originalWorkStart);
                }

                window.setMinimumSize(300, 300);
                window.resize(720, 300);
                QApplication::processEvents();
                expectTrue("short window enables inspector fallback scrolling",
                           inspector->verticalScrollBar()->maximum() > 0);
            }
        }
    }

    void testTodayNavigation()
    {
        AttendanceMainWindow window;
        auto* const calendar = window.findChild<CustomCalendarWidget*>();
        auto* const todayButton = window.findChild<QPushButton*>(u"todayButton"_s);
        auto* const statisticsPeriod = window.findChild<QLabel*>(u"statisticsPeriodLabel"_s);
        expectTrue("today navigation widgets exist", calendar && todayButton && statisticsPeriod);
        if (!calendar || !todayButton || !statisticsPeriod) {
            return;
        }

        window.show();
        QApplication::processEvents();
        const QDate today = QDate::currentDate();
        const auto verifyToday = [&](const char* const name) {
            todayButton->click();
            expectTrue(name,
                       calendar->selectedDate() == today && calendar->yearShown() == today.year() &&
                           calendar->monthShown() == today.month() &&
                           calendar->selectionMode() == QCalendarWidget::SingleSelection);
            QApplication::processEvents();
            expectTrue("today navigation refreshes statistics period",
                       statisticsPeriod->text() == u"%1年%2月"_s.arg(today.year()).arg(today.month()));
        };

        calendar->setSelectedDate(today);
        calendar->showPreviousMonth();
        verifyToday("today returns from previous month with unchanged selection");

        calendar->showNextMonth();
        calendar->clearDateSelection();
        verifyToday("today returns from next month with cleared selection");

        calendar->setSelectedDate(today.addDays(-1));
        calendar->showNextYear();
        verifyToday("today returns from another year with another selected date");

        calendar->showPreviousYear();
        verifyToday("today returns from another year with unchanged selection");
        verifyToday("repeated today clicks preserve the current page and selection");
    }

    void verifyCalendarDisplayRange(const char* const name,
                                    const QDate& month,
                                    const Qt::DayOfWeek firstDayOfWeek,
                                    const QDate& firstVisible,
                                    const QDate& lastVisible,
                                    const QDate& minimumDate = { },
                                    const QDate& maximumDate = { })
    {
        QSettings settings;
        settings.clear();
        const AttendanceRecord schedule;
        const std::array dates {
            firstVisible.addDays(-1),
            firstVisible,
            month.addDays(-1),
            month,
            month.addDays(month.daysInMonth() - 1),
            lastVisible,
            lastVisible.addDays(1),
        };
        for (const QDate& date : dates) {
            AttendanceRecord record = AttendanceSettings::createRecord(date, schedule);
            record.needAverageCal = true;
            expectTrue("display range fixture saves record", AttendanceSettings::saveRecord(settings, date, record));
        }

        {
            AttendanceMainWindow window;
            auto* const calendar = window.findChild<CustomCalendarWidget*>();
            auto* const workDays = window.findChild<QLabel*>(u"statsWorkDaysValueLabel"_s);
            expectTrue("display range widgets exist", calendar && workDays);
            if (!calendar || !workDays) {
                return;
            }

            calendar->setFirstDayOfWeek(firstDayOfWeek);
            if (minimumDate.isValid()) {
                calendar->setMinimumDate(minimumDate);
            }
            if (maximumDate.isValid()) {
                calendar->setMaximumDate(maximumDate);
            }
            calendar->setSelectedDate(month.addMonths(-1));
            calendar->setCurrentPage(month.year(), month.month());
            window.show();
            QApplication::processEvents();

            for (const QDate& date : dates) {
                const bool shouldBeVisible = date >= firstVisible && date <= lastVisible;
                const bool hasFormat = !calendar->dateTextFormat(date).isEmpty();
                const QByteArray message =
                    u"%1: record visibility on %2"_s.arg(QString::fromLatin1(name), date.toString(Qt::ISODate))
                        .toUtf8();
                expectTrue(message.constData(), hasFormat == shouldBeVisible);
            }
            expectTrue("adjacent month records do not count toward current month statistics",
                       workDays->text() == u"2天"_s);
        }

        settings.clear();
    }

    void testCalendarDisplayRanges()
    {
        verifyCalendarDisplayRange("Monday month start",
                                   QDate(2026, 6, 1),
                                   Qt::Monday,
                                   QDate(2026, 5, 25),
                                   QDate(2026, 7, 5));
        verifyCalendarDisplayRange("ordinary month start",
                                   QDate(2026, 7, 1),
                                   Qt::Monday,
                                   QDate(2026, 6, 29),
                                   QDate(2026, 8, 9));
        verifyCalendarDisplayRange("cross-year grid",
                                   QDate(2026, 1, 1),
                                   Qt::Monday,
                                   QDate(2025, 12, 29),
                                   QDate(2026, 2, 8));
        verifyCalendarDisplayRange("Sunday month start",
                                   QDate(2026, 3, 1),
                                   Qt::Sunday,
                                   QDate(2026, 2, 22),
                                   QDate(2026, 4, 4));
        verifyCalendarDisplayRange("minimum date clips without extending grid",
                                   QDate(2026, 6, 1),
                                   Qt::Monday,
                                   QDate(2026, 5, 28),
                                   QDate(2026, 7, 5),
                                   QDate(2026, 5, 28));
        verifyCalendarDisplayRange("maximum date clips grid",
                                   QDate(2026, 6, 1),
                                   Qt::Monday,
                                   QDate(2026, 5, 25),
                                   QDate(2026, 7, 3),
                                   { },
                                   QDate(2026, 7, 3));
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
    testTodayNavigation();
    testCalendarDisplayRanges();
    testCalendarPresentationSnapshot();
    testRecordActions();

    return failures == 0 ? 0 : 1;
}
