#include "Application/AttendanceMainWindow.h"
#include "Settings/AttendanceSettings.h"

#include <QApplication>
#include <QCheckBox>
#include <QLabel>
#include <QSettings>
#include <QTemporaryDir>

#include <iostream>

using namespace Qt::StringLiterals;

namespace {
    int failures = 0;

    QString activeSlotGroup(const QSettings& settings, const QString& group)
    {
        bool ok = false;
        const int slot = settings.value(group + u"/activeSlot"_s).toInt(&ok);
        return ok ? u"%1/slots/%2"_s.arg(group).arg(slot) : QString();
    }

    void writeLegacySchedule(QSettings& settings, const QString& group, const AttendanceRecord& schedule)
    {
        settings.setValue(group + u"/workStart"_s, schedule.workStartTime.toString(u"hh:mm"_s));
        settings.setValue(group + u"/workEnd"_s, schedule.workEndTime.toString(u"hh:mm"_s));
        settings.setValue(group + u"/lunchStart"_s, schedule.lunchBreakStart.toString(u"hh:mm"_s));
        settings.setValue(group + u"/lunchEnd"_s, schedule.lunchBreakEnd.toString(u"hh:mm"_s));
        settings.setValue(group + u"/dinnerStart"_s, schedule.dinnerBreakStart.toString(u"hh:mm"_s));
        settings.setValue(group + u"/dinnerEnd"_s, schedule.dinnerBreakEnd.toString(u"hh:mm"_s));
        settings.setValue(group + u"/mealSubsidy"_s, schedule.mealSubsidyTime.toString(u"hh:mm"_s));
    }

    void expectTrue(const char* name, bool value)
    {
        if (value) {
            return;
        }

        std::cerr << name << ": expected true\n";
        failures++;
    }

    void expectFalse(const char* name, bool value)
    {
        if (!value) {
            return;
        }

        std::cerr << name << ": expected false\n";
        failures++;
    }

    void expectEqual(const char* name, int actual, int expected)
    {
        if (actual == expected) {
            return;
        }

        std::cerr << name << ": expected " << expected << ", got " << actual << '\n';
        failures++;
    }

    void testSettingsStorage()
    {
        QSettings settings;
        settings.clear();

        AttendanceSettings::GlobalSettings globalSettings;
        globalSettings.schedule.workStartTime = QTime(8, 30);
        globalSettings.mealSubsidyEnabled = false;
        globalSettings.overtimeOffsetsMissingWork = true;
        globalSettings.targetOvertimeMinutes = 90;
        expectTrue("global settings save", AttendanceSettings::saveGlobalSettings(settings, globalSettings));

        const auto loadedGlobalSettings = AttendanceSettings::loadGlobalSettings(settings);
        expectTrue("global schedule round trip", loadedGlobalSettings.schedule.workStartTime == QTime(8, 30));
        expectFalse("meal subsidy round trip", loadedGlobalSettings.mealSubsidyEnabled);
        expectTrue("offset round trip", loadedGlobalSettings.overtimeOffsetsMissingWork);
        expectEqual("target round trip", loadedGlobalSettings.targetOvertimeMinutes, 90);

        const QDate weekday(2026, 8, 10);
        AttendanceRecord record = AttendanceSettings::createRecord(weekday, loadedGlobalSettings.schedule);
        record.arrivalTime = QTime(8, 15);
        record.departureTime = QTime(19, 30);
        expectTrue("record save", AttendanceSettings::saveRecord(settings, weekday, record));
        const QString weekdayGroup = weekday.toString(Qt::ISODate);
        expectTrue("record active slot", !activeSlotGroup(settings, weekdayGroup).isEmpty());
        expectTrue("record completion marker",
                   settings.value(activeSlotGroup(settings, weekdayGroup) + u"/complete"_s).toBool());

        const auto loadedRecord = AttendanceSettings::loadRecord(settings, weekday, loadedGlobalSettings.schedule);
        expectTrue("record round trip exists", loadedRecord.has_value());
        if (loadedRecord) {
            expectTrue("record arrival round trip", loadedRecord->arrivalTime == record.arrivalTime);
            expectTrue("record departure round trip", loadedRecord->departureTime == record.departureTime);
            expectTrue("record schedule round trip", loadedRecord->workStartTime == record.workStartTime);
        }

        expectTrue("record removal", AttendanceSettings::removeRecord(settings, weekday));
        expectFalse("record removed", AttendanceSettings::hasRecord(settings, weekday));

        const QDate legacyDate(2026, 8, 11);
        const QString legacyGroup = legacyDate.toString(Qt::ISODate);
        settings.setValue(legacyGroup + u"/arrival"_s, u"09:00"_s);
        settings.setValue(legacyGroup + u"/departure"_s, u"18:00"_s);
        expectTrue("legacy migration",
                   AttendanceSettings::migrateLegacyRecords(settings, loadedGlobalSettings.schedule));
        expectTrue("legacy schedule migrated",
                   settings.contains(activeSlotGroup(settings, legacyGroup) + u"/workStart"_s));
        expectTrue("legacy migration completion marker",
                   settings.value(activeSlotGroup(settings, legacyGroup) + u"/complete"_s).toBool());

        const QDate interruptedDate(2026, 8, 12);
        const QString interruptedGroup = interruptedDate.toString(Qt::ISODate);
        settings.setValue(interruptedGroup + u"/arrival"_s, u"09:30"_s);
        settings.setValue(interruptedGroup + u"/departure"_s, u"18:30"_s);
        settings.setValue(interruptedGroup + u"/complete"_s, false);
        settings.setValue(interruptedGroup + u"/workStart"_s, u"23:00"_s);
        settings.setValue(interruptedGroup + u"/workEnd"_s, u"08:00"_s);

        expectTrue("interrupted record migration",
                   AttendanceSettings::migrateLegacyRecords(settings, loadedGlobalSettings.schedule));
        expectTrue("interrupted record completion marker",
                   settings.value(activeSlotGroup(settings, interruptedGroup) + u"/complete"_s).toBool());
        const auto migratedInterruptedRecord =
            AttendanceSettings::loadRecord(settings, interruptedDate, loadedGlobalSettings.schedule);
        expectTrue("interrupted record loads after migration", migratedInterruptedRecord.has_value());
        if (migratedInterruptedRecord) {
            expectTrue("interrupted record uses fallback schedule",
                       migratedInterruptedRecord->workStartTime == loadedGlobalSettings.schedule.workStartTime);
        }

        AttendanceRecord changedSchedule = loadedGlobalSettings.schedule;
        changedSchedule.workStartTime = QTime(7, 45);
        expectTrue("repeat migration succeeds", AttendanceSettings::migrateLegacyRecords(settings, changedSchedule));
        const auto migratedLegacyRecord = AttendanceSettings::loadRecord(settings, legacyDate, changedSchedule);
        expectTrue("migrated legacy record still loads", migratedLegacyRecord.has_value());
        if (migratedLegacyRecord) {
            expectTrue("repeat migration preserves schedule snapshot",
                       migratedLegacyRecord->workStartTime == loadedGlobalSettings.schedule.workStartTime);
        }

        settings.clear();
    }

    void testVersionedRecoveryAndStrictSnapshots()
    {
        QSettings settings;
        settings.clear();

        AttendanceSettings::GlobalSettings firstGlobalSettings;
        firstGlobalSettings.schedule.workStartTime = QTime(8, 30);
        expectTrue("first versioned global save",
                   AttendanceSettings::saveGlobalSettings(settings, firstGlobalSettings));

        AttendanceSettings::GlobalSettings secondGlobalSettings = firstGlobalSettings;
        secondGlobalSettings.schedule.workStartTime = QTime(8, 0);
        expectTrue("second versioned global save",
                   AttendanceSettings::saveGlobalSettings(settings, secondGlobalSettings));

        const QString activeGlobalGroup = activeSlotGroup(settings, u"settings"_s);
        settings.setValue(activeGlobalGroup + u"/workStart"_s, u"invalid"_s);
        settings.sync();

        const AttendanceSettings::GlobalSettings recoveredGlobalSettings =
            AttendanceSettings::loadGlobalSettings(settings);
        expectTrue("corrupt global slot requires repair", recoveredGlobalSettings.requiresRepair);
        expectTrue("corrupt global slot recovers previous schedule",
                   recoveredGlobalSettings.schedule.workStartTime == firstGlobalSettings.schedule.workStartTime);

        const QDate recordDate(2026, 8, 17);
        AttendanceRecord firstRecord = AttendanceSettings::createRecord(recordDate, firstGlobalSettings.schedule);
        firstRecord.arrivalTime = QTime(8, 0);
        expectTrue("first versioned record save", AttendanceSettings::saveRecord(settings, recordDate, firstRecord));

        AttendanceRecord secondRecord = firstRecord;
        secondRecord.arrivalTime = QTime(8, 15);
        expectTrue("second versioned record save", AttendanceSettings::saveRecord(settings, recordDate, secondRecord));

        const QString recordGroup = recordDate.toString(Qt::ISODate);
        settings.setValue(activeSlotGroup(settings, recordGroup) + u"/workStart"_s, u"invalid"_s);
        settings.sync();

        AttendanceRecord unrelatedFallback;
        unrelatedFallback.workStartTime = QTime(7, 0);
        const auto recoveredRecord = AttendanceSettings::loadRecord(settings, recordDate, unrelatedFallback);
        expectTrue("corrupt record slot recovers previous record", recoveredRecord.has_value());
        if (recoveredRecord) {
            expectTrue("record recovery preserves previous arrival",
                       recoveredRecord->arrivalTime == firstRecord.arrivalTime);
            expectTrue("record recovery avoids current fallback",
                       recoveredRecord->workStartTime == firstRecord.workStartTime);
        }

        AttendanceRecord invalidMealSubsidyRecord;
        invalidMealSubsidyRecord.mealSubsidyTime = QTime();
        expectFalse("invalid meal subsidy time cannot save",
                    AttendanceSettings::saveRecord(settings, QDate(2026, 8, 18), invalidMealSubsidyRecord));

        const QDate corruptLegacyDate(2026, 8, 19);
        const QString corruptLegacyGroup = corruptLegacyDate.toString(Qt::ISODate);
        settings.setValue(corruptLegacyGroup + u"/arrival"_s, u"09:00"_s);
        settings.setValue(corruptLegacyGroup + u"/departure"_s, u"18:00"_s);
        settings.setValue(corruptLegacyGroup + u"/needAverageCal"_s, true);
        settings.setValue(corruptLegacyGroup + u"/complete"_s, true);
        writeLegacySchedule(settings, corruptLegacyGroup, AttendanceRecord { });
        settings.setValue(corruptLegacyGroup + u"/workStart"_s, u"invalid"_s);

        expectFalse("corrupt committed legacy record does not load",
                    AttendanceSettings::loadRecord(settings, corruptLegacyDate, AttendanceRecord { }).has_value());
        expectFalse("corrupt committed legacy record is not silently migrated",
                    AttendanceSettings::migrateLegacyRecords(settings, AttendanceRecord { }));
        expectFalse("corrupt committed legacy record has no active slot",
                    settings.contains(corruptLegacyGroup + u"/activeSlot"_s));
        expectTrue("corrupt committed legacy value is preserved",
                   settings.value(corruptLegacyGroup + u"/workStart"_s).toString() == u"invalid"_s);

        settings.clear();
    }

    void testIncompleteAndInvalidRecords()
    {
        QSettings settings;
        settings.clear();

        const QDate date(2026, 8, 12);
        const QString group = date.toString(Qt::ISODate);
        settings.setValue(group + u"/arrival"_s, u"09:00"_s);
        expectFalse("partial record has no record", AttendanceSettings::hasRecord(settings, date));
        expectFalse("partial record does not load",
                    AttendanceSettings::loadRecord(settings, date, AttendanceRecord { }).has_value());

        settings.setValue(group + u"/departure"_s, u"18:00"_s);
        expectTrue("complete legacy record loads",
                   AttendanceSettings::loadRecord(settings, date, AttendanceRecord { }).has_value());

        settings.setValue(group + u"/complete"_s, false);
        expectFalse("incomplete marker hides record", AttendanceSettings::hasRecord(settings, date));

        AttendanceRecord invalidRecord;
        invalidRecord.departureTime = invalidRecord.arrivalTime;
        expectFalse("invalid record cannot save", AttendanceSettings::saveRecord(settings, date, invalidRecord));

        AttendanceSettings::GlobalSettings incompleteGlobalSettings;
        incompleteGlobalSettings.schedule.workEndTime = incompleteGlobalSettings.schedule.workStartTime;
        expectFalse("invalid global settings cannot save",
                    AttendanceSettings::saveGlobalSettings(settings, incompleteGlobalSettings));

        settings.setValue(u"settings/complete"_s, false);
        const AttendanceSettings::GlobalSettings loadedIncompleteGlobalSettings =
            AttendanceSettings::loadGlobalSettings(settings);
        expectTrue("incomplete global settings require repair", loadedIncompleteGlobalSettings.requiresRepair);

        expectFalse("invalid date has no record", AttendanceSettings::hasRecord(settings, QDate()));
        expectFalse("invalid date cannot save",
                    AttendanceSettings::saveRecord(settings, QDate(), AttendanceRecord { }));
        expectFalse("invalid date cannot remove", AttendanceSettings::removeRecord(settings, QDate()));

        settings.clear();
    }

    void testMonthlyOffsetAcrossDates()
    {
        QSettings settings;
        settings.clear();

        AttendanceSettings::GlobalSettings globalSettings;
        globalSettings.overtimeOffsetsMissingWork = true;
        globalSettings.targetOvertimeMinutes = 0;
        expectTrue("monthly global settings save", AttendanceSettings::saveGlobalSettings(settings, globalSettings));

        const QDate firstDate(QDate::currentDate().year(), QDate::currentDate().month(), 1);
        AttendanceRecord missingWorkRecord = AttendanceSettings::createRecord(firstDate, globalSettings.schedule);
        missingWorkRecord.arrivalTime = QTime(10, 0);
        missingWorkRecord.departureTime = QTime(18, 0);
        expectTrue("monthly missing record save",
                   AttendanceSettings::saveRecord(settings, firstDate, missingWorkRecord));

        const QDate secondDate = firstDate.addDays(1);
        AttendanceRecord overtimeRecord = AttendanceSettings::createRecord(secondDate, globalSettings.schedule);
        overtimeRecord.arrivalTime = QTime(9, 0);
        overtimeRecord.departureTime = QTime(23, 0);
        expectTrue("monthly overtime record save",
                   AttendanceSettings::saveRecord(settings, secondDate, overtimeRecord));

        AttendanceMainWindow window;
        auto* overtimeLabel = window.findChild<QLabel*>(u"statsOvertimeValueLabel"_s);
        auto* missingWorkLabel = window.findChild<QLabel*>(u"statsMissingWorkValueLabel"_s);
        auto* checkBox = window.findChild<QCheckBox*>(u"overtimeOffsetsMissingWorkCheckBox"_s);
        expectTrue("monthly overtime value exists", overtimeLabel != nullptr);
        expectTrue("monthly missing work value exists", missingWorkLabel != nullptr);
        expectTrue("monthly offset checkbox exists", checkBox != nullptr);
        if (!overtimeLabel || !missingWorkLabel || !checkBox) {
            settings.clear();
            return;
        }

        expectTrue("cross-date offset remaining overtime", overtimeLabel->text() == u"3小时30分钟"_s);
        expectTrue("cross-date offset clears missing work", missingWorkLabel->text() == u"0分钟"_s);

        checkBox->setChecked(false);
        expectTrue("disabled cross-date offset keeps overtime", overtimeLabel->text() == u"4小时30分钟"_s);
        expectTrue("disabled cross-date offset keeps missing work", missingWorkLabel->text() == u"1小时"_s);

        settings.clear();
    }
} // namespace

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QTemporaryDir settingsDirectory;
    if (!settingsDirectory.isValid()) {
        std::cerr << "failed to create temporary settings directory\n";
        return 1;
    }

    QCoreApplication::setOrganizationName(u"AttendanceAppTests"_s);
    QCoreApplication::setApplicationName(u"AttendanceSettingsTests"_s);
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDirectory.path());

    testSettingsStorage();
    testVersionedRecoveryAndStrictSnapshots();
    testIncompleteAndInvalidRecords();

    {
        AttendanceMainWindow window;
        auto* checkBox = window.findChild<QCheckBox*>(u"overtimeOffsetsMissingWorkCheckBox"_s);
        expectTrue("offset checkbox exists", checkBox != nullptr);
        if (checkBox) {
            expectFalse("offset default", checkBox->isChecked());
            checkBox->setChecked(true);
        }
    }

    QSettings settings;
    settings.sync();
    expectTrue("offset setting persisted", AttendanceSettings::loadGlobalSettings(settings).overtimeOffsetsMissingWork);

    {
        AttendanceMainWindow window;
        auto* checkBox = window.findChild<QCheckBox*>(u"overtimeOffsetsMissingWorkCheckBox"_s);
        expectTrue("persisted offset checkbox exists", checkBox != nullptr);
        if (checkBox) {
            expectTrue("persisted offset value", checkBox->isChecked());
        }
    }

    testMonthlyOffsetAcrossDates();

    return failures == 0 ? 0 : 1;
}
