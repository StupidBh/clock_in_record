#include "Settings/AttendanceSettings.h"
#include "Attendance/WorkTimeCalculator.h"

#include <QLatin1StringView>

#include <array>
#include <vector>

using namespace Qt::StringLiterals;

namespace {
    constexpr auto ActiveSlotField = "activeSlot"_L1;
    constexpr auto CompletionField = "complete"_L1;
    constexpr auto SlotsGroup = "slots"_L1;
    constexpr auto StorageVersionKey = "metadata/storageVersion"_L1;
    constexpr int SlotCount = 2;
    constexpr int CurrentStorageVersion = 1;

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

    QString slotGroup(const QString& group, const int slot)
    {
        return u"%1/%2/%3"_s.arg(group, SlotsGroup).arg(slot);
    }

    QString activeSlotKey(const QString& group)
    {
        return groupKey(group, ActiveSlotField);
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

    void writeTime(QSettings& settings, const QString& key, const QTime& value)
    {
        settings.setValue(key, value.toString(u"hh:mm"_s));
    }

    void saveSchedule(QSettings& settings, const QString& group, const AttendanceRecord& schedule)
    {
        for (const auto& [name, value] : ScheduleFields) {
            writeTime(settings, groupKey(group, name), schedule.*value);
        }
    }

    bool loadScheduleStrict(const QSettings& settings, const QString& group, AttendanceRecord& schedule)
    {
        for (const auto& [name, value] : ScheduleFields) {
            const QString key = groupKey(group, name);
            if (!settings.contains(key)) {
                return false;
            }

            const QTime storedValue = parseTime(settings.value(key));
            if (!storedValue.isValid()) {
                return false;
            }
            schedule.*value = storedValue;
        }

        return WorkTimeCalculator::hasValidSchedule(schedule);
    }

    AttendanceRecord
        loadLegacySchedule(const QSettings& settings, const QString& group, const AttendanceRecord& fallback)
    {
        AttendanceRecord schedule;
        if (loadScheduleStrict(settings, group, schedule)) {
            return schedule;
        }
        return fallback;
    }

    std::optional<int> readActiveSlot(const QSettings& settings, const QString& group)
    {
        const QString key = activeSlotKey(group);
        if (!settings.contains(key)) {
            return std::nullopt;
        }

        bool ok = false;
        const int slot = settings.value(key).toInt(&ok);
        if (!ok || slot < 0 || slot >= SlotCount) {
            return std::nullopt;
        }
        return slot;
    }

    void writeRecordValues(QSettings& settings, const QString& group, const AttendanceRecord& record)
    {
        settings.setValue(groupKey(group, "needAverageCal"_L1), record.needAverageCal);
        writeTime(settings, groupKey(group, "arrival"_L1), record.arrivalTime);
        writeTime(settings, groupKey(group, "departure"_L1), record.departureTime);
        saveSchedule(settings, group, record);
    }

    std::optional<AttendanceRecord> loadRecordSlot(const QSettings& settings, const QString& group, const int slot)
    {
        const QString storedGroup = slotGroup(group, slot);
        if (!settings.value(completionKey(storedGroup), false).toBool() ||
            !settings.contains(groupKey(storedGroup, "needAverageCal"_L1)) ||
            !settings.contains(groupKey(storedGroup, "arrival"_L1)) ||
            !settings.contains(groupKey(storedGroup, "departure"_L1))) {
            return std::nullopt;
        }

        AttendanceRecord record;
        if (!loadScheduleStrict(settings, storedGroup, record)) {
            return std::nullopt;
        }

        record.needAverageCal = settings.value(groupKey(storedGroup, "needAverageCal"_L1)).toBool();
        record.arrivalTime = parseTime(settings.value(groupKey(storedGroup, "arrival"_L1)));
        record.departureTime = parseTime(settings.value(groupKey(storedGroup, "departure"_L1)));
        if (!WorkTimeCalculator::hasValidAttendanceRange(record)) {
            return std::nullopt;
        }
        return record;
    }

    std::optional<AttendanceRecord> loadBestRecordSlot(const QSettings& settings, const QString& group)
    {
        const std::optional<int> activeSlot = readActiveSlot(settings, group);
        if (activeSlot) {
            if (const auto activeRecord = loadRecordSlot(settings, group, *activeSlot)) {
                return activeRecord;
            }
        }

        for (int slot = 0; slot < SlotCount; ++slot) {
            if (activeSlot && slot == *activeSlot) {
                continue;
            }
            if (const auto fallbackRecord = loadRecordSlot(settings, group, slot)) {
                return fallbackRecord;
            }
        }
        return std::nullopt;
    }

    std::optional<int> findReadableRecordSlot(const QSettings& settings, const QString& group)
    {
        const std::optional<int> activeSlot = readActiveSlot(settings, group);
        if (activeSlot && loadRecordSlot(settings, group, *activeSlot)) {
            return activeSlot;
        }

        for (int slot = 0; slot < SlotCount; ++slot) {
            if (activeSlot && slot == *activeSlot) {
                continue;
            }
            if (loadRecordSlot(settings, group, slot)) {
                return slot;
            }
        }
        return std::nullopt;
    }

    std::optional<AttendanceSettings::GlobalSettings>
        loadGlobalSlot(const QSettings& settings, const QString& group, const int slot)
    {
        const QString storedGroup = slotGroup(group, slot);
        if (!settings.value(completionKey(storedGroup), false).toBool() ||
            !settings.contains(groupKey(storedGroup, "mealSubsidyEnabled"_L1)) ||
            !settings.contains(groupKey(storedGroup, "overtimeOffsetsMissingWork"_L1)) ||
            !settings.contains(groupKey(storedGroup, "targetOvertimeMinutes"_L1))) {
            return std::nullopt;
        }

        AttendanceSettings::GlobalSettings globalSettings;
        if (!loadScheduleStrict(settings, storedGroup, globalSettings.schedule)) {
            return std::nullopt;
        }

        globalSettings.mealSubsidyEnabled = settings.value(groupKey(storedGroup, "mealSubsidyEnabled"_L1)).toBool();
        globalSettings.overtimeOffsetsMissingWork =
            settings.value(groupKey(storedGroup, "overtimeOffsetsMissingWork"_L1)).toBool();

        bool targetOk = false;
        globalSettings.targetOvertimeMinutes =
            settings.value(groupKey(storedGroup, "targetOvertimeMinutes"_L1)).toInt(&targetOk);
        if (!targetOk || globalSettings.targetOvertimeMinutes < 0 || globalSettings.targetOvertimeMinutes > 24 * 60) {
            return std::nullopt;
        }
        return globalSettings;
    }

    std::optional<int> findReadableGlobalSlot(const QSettings& settings, const QString& group)
    {
        const std::optional<int> activeSlot = readActiveSlot(settings, group);
        if (activeSlot && loadGlobalSlot(settings, group, *activeSlot)) {
            return activeSlot;
        }

        for (int slot = 0; slot < SlotCount; ++slot) {
            if (activeSlot && slot == *activeSlot) {
                continue;
            }
            if (loadGlobalSlot(settings, group, slot)) {
                return slot;
            }
        }
        return std::nullopt;
    }

    int inactiveSlot(const std::optional<int> activeSlot)
    {
        return activeSlot && *activeSlot == 0 ? 1 : 0;
    }

    int stageRecord(QSettings& settings, const QString& group, const AttendanceRecord& record)
    {
        const int targetSlot = inactiveSlot(findReadableRecordSlot(settings, group));
        const QString targetGroup = slotGroup(group, targetSlot);
        settings.remove(targetGroup);
        writeRecordValues(settings, targetGroup, record);
        settings.setValue(completionKey(targetGroup), true);
        return targetSlot;
    }
} // namespace

namespace AttendanceSettings {
    GlobalSettings loadGlobalSettings(const QSettings& settings)
    {
        const QString settingsGroup = u"settings"_s;
        if (settings.contains(activeSlotKey(settingsGroup))) {
            const std::optional<int> requestedSlot = readActiveSlot(settings, settingsGroup);
            if (requestedSlot) {
                if (const auto storedSettings = loadGlobalSlot(settings, settingsGroup, *requestedSlot)) {
                    return *storedSettings;
                }
            }

            if (const std::optional<int> fallbackSlot = findReadableGlobalSlot(settings, settingsGroup)) {
                GlobalSettings recoveredSettings = *loadGlobalSlot(settings, settingsGroup, *fallbackSlot);
                recoveredSettings.requiresRepair = true;
                return recoveredSettings;
            }

            GlobalSettings defaults;
            defaults.requiresRepair = true;
            return defaults;
        }

        GlobalSettings globalSettings;
        if (settings.contains(completionKey(settingsGroup)) && !settings.value(completionKey(settingsGroup)).toBool()) {
            globalSettings.requiresRepair = true;
            return globalSettings;
        }

        const AttendanceRecord defaults;
        AttendanceRecord legacySchedule = defaults;
        bool hasAnyScheduleField = false;
        bool hasMissingOrInvalidField = false;
        for (const auto& [name, value] : ScheduleFields) {
            const QString key = groupKey(settingsGroup, name);
            if (!settings.contains(key)) {
                continue;
            }

            hasAnyScheduleField = true;
            const QTime storedValue = parseTime(settings.value(key));
            if (!storedValue.isValid()) {
                hasMissingOrInvalidField = true;
                continue;
            }
            legacySchedule.*value = storedValue;
        }

        if (hasAnyScheduleField) {
            for (const auto& [name, value] : ScheduleFields) {
                if (!settings.contains(groupKey(settingsGroup, name))) {
                    hasMissingOrInvalidField = true;
                }
            }
        }

        if (!WorkTimeCalculator::hasValidSchedule(legacySchedule)) {
            legacySchedule = defaults;
            hasMissingOrInvalidField = true;
        }
        globalSettings.schedule = legacySchedule;
        globalSettings.requiresRepair = hasMissingOrInvalidField;

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
        const int targetSlot = inactiveSlot(findReadableGlobalSlot(settings, settingsGroup));
        const QString targetGroup = slotGroup(settingsGroup, targetSlot);
        settings.remove(targetGroup);
        saveSchedule(settings, targetGroup, globalSettings.schedule);
        settings.setValue(groupKey(targetGroup, "mealSubsidyEnabled"_L1), globalSettings.mealSubsidyEnabled);
        settings.setValue(groupKey(targetGroup, "overtimeOffsetsMissingWork"_L1),
                          globalSettings.overtimeOffsetsMissingWork);
        settings.setValue(groupKey(targetGroup, "targetOvertimeMinutes"_L1), globalSettings.targetOvertimeMinutes);
        settings.setValue(completionKey(targetGroup), true);
        if (!syncSuccessfully(settings) || !loadGlobalSlot(settings, settingsGroup, targetSlot)) {
            return false;
        }

        settings.setValue(activeSlotKey(settingsGroup), targetSlot);
        return syncSuccessfully(settings);
    }

    bool hasRecord(const QSettings& settings, const QDate& date)
    {
        if (!date.isValid()) {
            return false;
        }

        const QString group = recordGroup(date);
        if (settings.contains(activeSlotKey(group))) {
            return loadBestRecordSlot(settings, group).has_value();
        }
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
        if (!date.isValid()) {
            return std::nullopt;
        }

        const QString group = recordGroup(date);
        if (settings.contains(activeSlotKey(group))) {
            return loadBestRecordSlot(settings, group);
        }
        if (!hasRecord(settings, date)) {
            return std::nullopt;
        }

        AttendanceRecord record;
        const bool explicitlyComplete = settings.contains(completionKey(group));
        if (explicitlyComplete) {
            if (!settings.value(completionKey(group)).toBool() ||
                !settings.contains(groupKey(group, "needAverageCal"_L1)) ||
                !loadScheduleStrict(settings, group, record)) {
                return std::nullopt;
            }
        }
        else {
            record = loadLegacySchedule(settings, group, scheduleFallback);
        }

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
        const int targetSlot = stageRecord(settings, group, record);
        if (!syncSuccessfully(settings) || !loadRecordSlot(settings, group, targetSlot)) {
            return false;
        }

        settings.setValue(activeSlotKey(group), targetSlot);
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

        bool versionOk = false;
        const int storedVersion = settings.value(StorageVersionKey).toInt(&versionOk);
        if (versionOk && storedVersion >= CurrentStorageVersion) {
            return true;
        }

        struct StagedRecord
        {
            QString group;
            int slot;
        };

        bool success = true;
        std::vector<StagedRecord> stagedRecords;
        for (const QString& group : settings.childGroups()) {
            const QDate date = QDate::fromString(group, Qt::ISODate);
            if (!date.isValid() || settings.contains(activeSlotKey(group)) ||
                !settings.contains(groupKey(group, "arrival"_L1)) ||
                !settings.contains(groupKey(group, "departure"_L1))) {
                continue;
            }

            AttendanceRecord record;
            const bool hasCompletionMarker = settings.contains(completionKey(group));
            const bool markedComplete = settings.value(completionKey(group), false).toBool();
            if (hasCompletionMarker && markedComplete) {
                if (!settings.contains(groupKey(group, "needAverageCal"_L1)) ||
                    !loadScheduleStrict(settings, group, record)) {
                    success = false;
                    continue;
                }
            }
            else if (hasCompletionMarker) {
                record = currentSchedule;
            }
            else {
                record = loadLegacySchedule(settings, group, currentSchedule);
            }

            const AttendanceRecord defaults = createRecord(date, currentSchedule);
            record.needAverageCal =
                settings.value(groupKey(group, "needAverageCal"_L1), defaults.needAverageCal).toBool();
            record.arrivalTime = parseTime(settings.value(groupKey(group, "arrival"_L1)));
            record.departureTime = parseTime(settings.value(groupKey(group, "departure"_L1)));
            if (!WorkTimeCalculator::hasValidAttendanceRange(record)) {
                continue;
            }

            if (!WorkTimeCalculator::hasValidSchedule(record)) {
                success = false;
                continue;
            }

            stagedRecords.push_back({ .group = group, .slot = stageRecord(settings, group, record) });
        }

        if (!stagedRecords.empty()) {
            if (!syncSuccessfully(settings)) {
                return false;
            }

            std::vector<StagedRecord> verifiedRecords;
            verifiedRecords.reserve(stagedRecords.size());
            for (const StagedRecord& stagedRecord : stagedRecords) {
                if (loadRecordSlot(settings, stagedRecord.group, stagedRecord.slot)) {
                    verifiedRecords.push_back(stagedRecord);
                }
                else {
                    success = false;
                }
            }

            for (const StagedRecord& verifiedRecord : verifiedRecords) {
                settings.setValue(activeSlotKey(verifiedRecord.group), verifiedRecord.slot);
            }
            if (!verifiedRecords.empty() && !syncSuccessfully(settings)) {
                return false;
            }
        }

        if (!success) {
            return false;
        }

        settings.setValue(StorageVersionKey, CurrentStorageVersion);
        return syncSuccessfully(settings);
    }
} // namespace AttendanceSettings
