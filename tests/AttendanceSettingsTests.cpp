#include "Application/AttendanceMainWindow.h"
#include "Settings/AttendanceSettings.h"

#include <QApplication>
#include <QCheckBox>
#include <QSettings>
#include <QTemporaryDir>

#include <iostream>

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
        AttendanceSettings::saveGlobalSettings(settings, globalSettings);

        const auto loadedGlobalSettings = AttendanceSettings::loadGlobalSettings(settings);
        expectTrue("global schedule round trip", loadedGlobalSettings.schedule.workStartTime == QTime(8, 30));
        expectFalse("meal subsidy round trip", loadedGlobalSettings.mealSubsidyEnabled);
        expectTrue("offset round trip", loadedGlobalSettings.overtimeOffsetsMissingWork);
        expectEqual("target round trip", loadedGlobalSettings.targetOvertimeMinutes, 90);

        const QDate weekday(2026, 8, 10);
        AttendanceRecord record = AttendanceSettings::createRecord(weekday, loadedGlobalSettings.schedule);
        record.arrivalTime = QTime(8, 15);
        record.departureTime = QTime(19, 30);
        AttendanceSettings::saveRecord(settings, weekday, record);

        const auto loadedRecord = AttendanceSettings::loadRecord(settings, weekday, loadedGlobalSettings.schedule);
        expectTrue("record round trip exists", loadedRecord.has_value());
        if (loadedRecord) {
            expectTrue("record arrival round trip", loadedRecord->arrivalTime == record.arrivalTime);
            expectTrue("record departure round trip", loadedRecord->departureTime == record.departureTime);
            expectTrue("record schedule round trip", loadedRecord->workStartTime == record.workStartTime);
        }

        AttendanceSettings::removeRecord(settings, weekday);
        expectFalse("record removed", AttendanceSettings::hasRecord(settings, weekday));

        const QDate legacyDate(2026, 8, 11);
        const QString legacyGroup = legacyDate.toString(Qt::ISODate);
        settings.setValue(legacyGroup + "/arrival", "09:00");
        settings.setValue(legacyGroup + "/departure", "18:00");
        AttendanceSettings::migrateLegacyRecords(settings, loadedGlobalSettings.schedule);
        expectTrue("legacy schedule migrated", settings.contains(legacyGroup + "/workStart"));

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

    QCoreApplication::setOrganizationName("AttendanceAppTests");
    QCoreApplication::setApplicationName("AttendanceSettingsTests");
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDirectory.path());

    testSettingsStorage();

    {
        AttendanceMainWindow window;
        auto* checkBox = window.findChild<QCheckBox*>("overtimeOffsetsMissingWorkCheckBox");
        expectTrue("offset checkbox exists", checkBox != nullptr);
        if (checkBox) {
            expectFalse("offset default", checkBox->isChecked());
            checkBox->setChecked(true);
        }
    }

    QSettings settings;
    settings.sync();
    expectTrue("offset setting persisted", settings.value("settings/overtimeOffsetsMissingWork", false).toBool());

    {
        AttendanceMainWindow window;
        auto* checkBox = window.findChild<QCheckBox*>("overtimeOffsetsMissingWorkCheckBox");
        expectTrue("persisted offset checkbox exists", checkBox != nullptr);
        if (checkBox) {
            expectTrue("persisted offset value", checkBox->isChecked());
        }
    }

    return failures == 0 ? 0 : 1;
}
