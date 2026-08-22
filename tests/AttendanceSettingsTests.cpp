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
        expectTrue("record completion marker", settings.value(weekday.toString(Qt::ISODate) + u"/complete"_s).toBool());

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
        expectTrue("legacy schedule migrated", settings.contains(legacyGroup + u"/workStart"_s));

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
        auto* statisticsLabel = window.findChild<QLabel*>(u"monthlyStatisticsLabel"_s);
        auto* checkBox = window.findChild<QCheckBox*>(u"overtimeOffsetsMissingWorkCheckBox"_s);
        expectTrue("monthly statistics label exists", statisticsLabel != nullptr);
        expectTrue("monthly offset checkbox exists", checkBox != nullptr);
        if (!statisticsLabel || !checkBox) {
            settings.clear();
            return;
        }

        expectTrue("cross-date offset remaining overtime",
                   statisticsLabel->text().contains(u"总加班时长: 3小时30分钟"_s));
        expectFalse("cross-date offset clears missing work", statisticsLabel->text().contains(u"缺少标准工时"_s));

        checkBox->setChecked(false);
        expectTrue("disabled cross-date offset keeps overtime",
                   statisticsLabel->text().contains(u"总加班时长: 4小时30分钟"_s));
        expectTrue("disabled cross-date offset keeps missing work",
                   statisticsLabel->text().contains(u"缺少标准工时: 1小时0分钟"_s));

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
    expectTrue("offset setting persisted", settings.value(u"settings/overtimeOffsetsMissingWork"_s, false).toBool());

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
