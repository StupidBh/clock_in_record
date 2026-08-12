#include "Settings/AttendanceSettings.h"

#include "Attendance/WorkTimeCalculator.h"

#include <array>

namespace {
    constexpr auto TimeFormat = "hh:mm";
    constexpr auto GlobalSettingsGroup = "settings";

    struct ScheduleField
    {
        const char* name;
        QTime AttendanceRecord::* value;
    };

    constexpr std::array ScheduleFields {
        ScheduleField { .name = "workStart", .value = &AttendanceRecord::workStartTime },
        ScheduleField { .name = "workEnd", .value = &AttendanceRecord::workEndTime },
        ScheduleField { .name = "lunchStart", .value = &AttendanceRecord::lunchBreakStart },
        ScheduleField { .name = "lunchEnd", .value = &AttendanceRecord::lunchBreakEnd },
        ScheduleField { .name = "dinnerStart", .value = &AttendanceRecord::dinnerBreakStart },
        ScheduleField { .name = "dinnerEnd", .value = &AttendanceRecord::dinnerBreakEnd },
        ScheduleField { .name = "mealSubsidy", .value = &AttendanceRecord::mealSubsidyTime },
    };

    QString recordGroup(const QDate& date)
    {
        return date.toString(Qt::ISODate);
    }

    QString groupKey(const QString& group, const char* name)
    {
        return group + QLatin1Char('/') + QLatin1StringView(name);
    }

    QTime parseTime(const QVariant& value)
    {
        return QTime::fromString(value.toString(), TimeFormat);
    }

    QTime readTime(const QSettings& settings, const QString& key, const QTime& fallback)
    {
        const QTime value = parseTime(settings.value(key, fallback.toString(TimeFormat)));
        return value.isValid() ? value : fallback;
    }

    void writeTime(QSettings& settings, const QString& key, const QTime& value)
    {
        settings.setValue(key, value.toString(TimeFormat));
    }

    AttendanceRecord loadScheduleUnchecked(const QSettings& settings, const QString& group, const AttendanceRecord& fallback)
    {
        AttendanceRecord schedule = fallback;
        for (const auto& [name, value] : ScheduleFields) {
            schedule.*value = readTime(settings, groupKey(group, name), fallback.*value);
        }

        return schedule;
    }

    AttendanceRecord loadSchedule(const QSettings& settings, const QString& group, const AttendanceRecord& fallback)
    {
        AttendanceRecord schedule = loadScheduleUnchecked(settings, group, fallback);
        if (WorkTimeCalculator::hasValidSchedule(schedule)) {
            return schedule;
        }

        const QTime mealSubsidyTime = schedule.mealSubsidyTime;
        schedule = fallback;
        schedule.mealSubsidyTime = mealSubsidyTime;
        return schedule;
    }

    void saveSchedule(QSettings& settings, const QString& group, const AttendanceRecord& schedule)
    {
        for (const auto& [name, value] : ScheduleFields) {
            writeTime(settings, groupKey(group, name), schedule.*value);
        }
    }
} // namespace

namespace AttendanceSettings {
    GlobalSettings loadGlobalSettings(const QSettings& settings)
    {
        GlobalSettings globalSettings;
        const AttendanceRecord defaults;
        const AttendanceRecord rawSchedule = loadScheduleUnchecked(settings, QLatin1String(GlobalSettingsGroup), defaults);
        globalSettings.requiresRepair = !WorkTimeCalculator::hasValidSchedule(rawSchedule);
        globalSettings.schedule = globalSettings.requiresRepair ? defaults : rawSchedule;

        globalSettings.mealSubsidyEnabled = settings.value("settings/mealSubsidyEnabled", true).toBool();
        globalSettings.overtimeOffsetsMissingWork = settings.value("settings/overtimeOffsetsMissingWork", false).toBool();

        bool targetOk = false;
        globalSettings.targetOvertimeMinutes = settings.value("settings/targetOvertimeMinutes", 150).toInt(&targetOk);
        if (!targetOk || globalSettings.targetOvertimeMinutes < 0 || globalSettings.targetOvertimeMinutes > 24 * 60) {
            globalSettings.targetOvertimeMinutes = 150;
            globalSettings.requiresRepair = true;
        }

        return globalSettings;
    }

    void saveGlobalSettings(QSettings& settings, const GlobalSettings& globalSettings)
    {
        saveSchedule(settings, QLatin1String(GlobalSettingsGroup), globalSettings.schedule);
        settings.setValue("settings/mealSubsidyEnabled", globalSettings.mealSubsidyEnabled);
        settings.setValue("settings/overtimeOffsetsMissingWork", globalSettings.overtimeOffsetsMissingWork);
        settings.setValue("settings/targetOvertimeMinutes", globalSettings.targetOvertimeMinutes);
    }

    bool hasRecord(const QSettings& settings, const QDate& date)
    {
        return settings.contains(groupKey(recordGroup(date), "arrival"));
    }

    AttendanceRecord createRecord(const QDate& date, const AttendanceRecord& schedule)
    {
        AttendanceRecord record = schedule;
        const int dayOfWeek = date.dayOfWeek();
        record.needAverageCal = dayOfWeek != Qt::Saturday && dayOfWeek != Qt::Sunday;
        record.arrivalTime = AttendanceRecord { }.arrivalTime;
        record.departureTime = AttendanceRecord { }.departureTime;
        return record;
    }

    std::optional<AttendanceRecord> loadRecord(const QSettings& settings, const QDate& date, const AttendanceRecord& scheduleFallback)
    {
        if (!hasRecord(settings, date)) {
            return std::nullopt;
        }

        const QString group = recordGroup(date);
        AttendanceRecord record = loadSchedule(settings, group, scheduleFallback);
        const AttendanceRecord defaults = createRecord(date, scheduleFallback);
        record.needAverageCal = settings.value(groupKey(group, "needAverageCal"), defaults.needAverageCal).toBool();
        record.arrivalTime = parseTime(settings.value(groupKey(group, "arrival")));
        record.departureTime = parseTime(settings.value(groupKey(group, "departure")));
        return record;
    }

    void saveRecord(QSettings& settings, const QDate& date, const AttendanceRecord& record)
    {
        const QString group = recordGroup(date);
        settings.setValue(groupKey(group, "needAverageCal"), record.needAverageCal);
        writeTime(settings, groupKey(group, "arrival"), record.arrivalTime);
        writeTime(settings, groupKey(group, "departure"), record.departureTime);
        saveSchedule(settings, group, record);
    }

    void removeRecord(QSettings& settings, const QDate& date)
    {
        settings.remove(recordGroup(date));
    }

    void migrateLegacyRecords(QSettings& settings, const AttendanceRecord& currentSchedule)
    {
        for (const QString& group : settings.childGroups()) {
            if (!QDate::fromString(group, Qt::ISODate).isValid() || !settings.contains(groupKey(group, "arrival"))) {
                continue;
            }

            const AttendanceRecord storedSchedule = loadSchedule(settings, group, currentSchedule);
            saveSchedule(settings, group, storedSchedule);
        }
    }
} // namespace AttendanceSettings
