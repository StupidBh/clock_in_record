#include "Settings/AttendanceSettings.h"
#include "Attendance/WorkTimeCalculator.h"

#include <QLatin1StringView>

#include <array>

using namespace Qt::StringLiterals;

namespace {
    constexpr auto CompletionField = "complete"_L1;

    struct ScheduleField
    {
        QLatin1StringView name;
        QTime AttendanceRecord::* value;
    };

    constexpr std::array ScheduleFields {
        ScheduleField { .name = "workStart"_L1, .value = &AttendanceRecord::workStartTime },
        ScheduleField { .name = "workEnd"_L1, .value = &AttendanceRecord::workEndTime },
        ScheduleField { .name = "lunchStart"_L1, .value = &AttendanceRecord::lunchBreakStart },
        ScheduleField { .name = "lunchEnd"_L1, .value = &AttendanceRecord::lunchBreakEnd },
        ScheduleField { .name = "dinnerStart"_L1, .value = &AttendanceRecord::dinnerBreakStart },
        ScheduleField { .name = "dinnerEnd"_L1, .value = &AttendanceRecord::dinnerBreakEnd },
        ScheduleField { .name = "mealSubsidy"_L1, .value = &AttendanceRecord::mealSubsidyTime },
    };

    QString recordGroup(const QDate& date)
    {
        return date.toString(Qt::ISODate);
    }

    QString groupKey(const QString& group, const QLatin1StringView name)
    {
        QString key = group;
        key += u'/';
        key += name;
        return key;
    }

    QString completionKey(const QString& group)
    {
        return groupKey(group, CompletionField);
    }

    bool syncSuccessfully(QSettings& settings)
    {
        settings.sync();
        return settings.status() == QSettings::NoError;
    }

    QTime parseTime(const QVariant& value)
    {
        return QTime::fromString(value.toString(), u"hh:mm"_s);
    }

    QTime readTime(const QSettings& settings, const QString& key, const QTime& fallback)
    {
        const QTime value = parseTime(settings.value(key, fallback.toString(u"hh:mm"_s)));
        return value.isValid() ? value : fallback;
    }

    void writeTime(QSettings& settings, const QString& key, const QTime& value)
    {
        settings.setValue(key, value.toString(u"hh:mm"_s));
    }

    AttendanceRecord
        loadScheduleUnchecked(const QSettings& settings, const QString& group, const AttendanceRecord& fallback)
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
        const QString settingsGroup = u"settings"_s;
        if (settings.contains(completionKey(settingsGroup)) && !settings.value(completionKey(settingsGroup)).toBool()) {
            globalSettings.requiresRepair = true;
            return globalSettings;
        }

        const AttendanceRecord defaults;
        const AttendanceRecord rawSchedule = loadScheduleUnchecked(settings, settingsGroup, defaults);
        globalSettings.requiresRepair = !WorkTimeCalculator::hasValidSchedule(rawSchedule);
        globalSettings.schedule = globalSettings.requiresRepair ? defaults : rawSchedule;

        globalSettings.mealSubsidyEnabled =
            settings.value(groupKey(settingsGroup, "mealSubsidyEnabled"_L1), true).toBool();
        globalSettings.overtimeOffsetsMissingWork =
            settings.value(groupKey(settingsGroup, "overtimeOffsetsMissingWork"_L1), false).toBool();

        bool targetOk = false;
        globalSettings.targetOvertimeMinutes =
            settings.value(groupKey(settingsGroup, "targetOvertimeMinutes"_L1), 150).toInt(&targetOk);
        if (!targetOk || globalSettings.targetOvertimeMinutes < 0 || globalSettings.targetOvertimeMinutes > 24 * 60) {
            globalSettings.targetOvertimeMinutes = 150;
            globalSettings.requiresRepair = true;
        }

        return globalSettings;
    }

    bool saveGlobalSettings(QSettings& settings, const GlobalSettings& globalSettings)
    {
        if (!WorkTimeCalculator::hasValidSchedule(globalSettings.schedule) ||
            globalSettings.targetOvertimeMinutes < 0 || globalSettings.targetOvertimeMinutes > 24 * 60) {
            return false;
        }

        const QString settingsGroup = u"settings"_s;
        settings.setValue(completionKey(settingsGroup), false);
        saveSchedule(settings, settingsGroup, globalSettings.schedule);
        settings.setValue(groupKey(settingsGroup, "mealSubsidyEnabled"_L1), globalSettings.mealSubsidyEnabled);
        settings.setValue(groupKey(settingsGroup, "overtimeOffsetsMissingWork"_L1),
                          globalSettings.overtimeOffsetsMissingWork);
        settings.setValue(groupKey(settingsGroup, "targetOvertimeMinutes"_L1), globalSettings.targetOvertimeMinutes);
        if (!syncSuccessfully(settings)) {
            return false;
        }

        settings.setValue(completionKey(settingsGroup), true);
        return syncSuccessfully(settings);
    }

    bool hasRecord(const QSettings& settings, const QDate& date)
    {
        if (!date.isValid()) {
            return false;
        }

        const QString group = recordGroup(date);
        if (settings.contains(completionKey(group)) && !settings.value(completionKey(group)).toBool()) {
            return false;
        }

        const QString arrivalKey = groupKey(group, "arrival"_L1);
        const QString departureKey = groupKey(group, "departure"_L1);
        if (!settings.contains(arrivalKey) || !settings.contains(departureKey)) {
            return false;
        }

        const QTime arrival = parseTime(settings.value(arrivalKey));
        const QTime departure = parseTime(settings.value(departureKey));
        return arrival.isValid() && departure.isValid() && arrival < departure;
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

    std::optional<AttendanceRecord>
        loadRecord(const QSettings& settings, const QDate& date, const AttendanceRecord& scheduleFallback)
    {
        if (!date.isValid() || !hasRecord(settings, date)) {
            return std::nullopt;
        }

        const QString group = recordGroup(date);
        AttendanceRecord record = loadSchedule(settings, group, scheduleFallback);
        const AttendanceRecord defaults = createRecord(date, scheduleFallback);
        record.needAverageCal = settings.value(groupKey(group, "needAverageCal"_L1), defaults.needAverageCal).toBool();
        record.arrivalTime = parseTime(settings.value(groupKey(group, "arrival"_L1)));
        record.departureTime = parseTime(settings.value(groupKey(group, "departure"_L1)));
        if (!WorkTimeCalculator::hasValidAttendanceRange(record) || !WorkTimeCalculator::hasValidSchedule(record)) {
            return std::nullopt;
        }
        return record;
    }

    bool saveRecord(QSettings& settings, const QDate& date, const AttendanceRecord& record)
    {
        if (!date.isValid() || !WorkTimeCalculator::hasValidAttendanceRange(record) ||
            !WorkTimeCalculator::hasValidSchedule(record)) {
            return false;
        }

        const QString group = recordGroup(date);
        settings.setValue(completionKey(group), false);
        settings.setValue(groupKey(group, "needAverageCal"_L1), record.needAverageCal);
        writeTime(settings, groupKey(group, "arrival"_L1), record.arrivalTime);
        writeTime(settings, groupKey(group, "departure"_L1), record.departureTime);
        saveSchedule(settings, group, record);
        if (!syncSuccessfully(settings)) {
            return false;
        }

        settings.setValue(completionKey(group), true);
        return syncSuccessfully(settings);
    }

    bool removeRecord(QSettings& settings, const QDate& date)
    {
        if (!date.isValid()) {
            return false;
        }

        settings.remove(recordGroup(date));
        return syncSuccessfully(settings);
    }

    bool migrateLegacyRecords(QSettings& settings, const AttendanceRecord& currentSchedule)
    {
        if (!WorkTimeCalculator::hasValidSchedule(currentSchedule)) {
            return false;
        }

        bool success = true;
        for (const QString& group : settings.childGroups()) {
            const QDate date = QDate::fromString(group, Qt::ISODate);
            if (!date.isValid() || !settings.contains(groupKey(group, "arrival"_L1)) ||
                !settings.contains(groupKey(group, "departure"_L1))) {
                continue;
            }

            AttendanceRecord record = loadSchedule(settings, group, currentSchedule);
            const AttendanceRecord defaults = createRecord(date, currentSchedule);
            record.needAverageCal =
                settings.value(groupKey(group, "needAverageCal"_L1), defaults.needAverageCal).toBool();
            record.arrivalTime = parseTime(settings.value(groupKey(group, "arrival"_L1)));
            record.departureTime = parseTime(settings.value(groupKey(group, "departure"_L1)));
            if (!WorkTimeCalculator::hasValidAttendanceRange(record)) {
                continue;
            }

            if (!saveRecord(settings, date, record)) {
                success = false;
            }
        }

        return success;
    }
} // namespace AttendanceSettings
