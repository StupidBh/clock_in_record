#pragma once
#include "Attendance/AttendanceTypes.h"

#include <QDate>
#include <QSettings>

#include <optional>

namespace AttendanceSettings {
    struct GlobalSettings
    {
        AttendanceRecord schedule;
        bool mealSubsidyEnabled = true;
        bool overtimeOffsetsMissingWork = false;
        int targetOvertimeMinutes = 150;
        bool requiresRepair = false;
    };

    [[nodiscard]] GlobalSettings loadGlobalSettings(const QSettings& settings);
    [[nodiscard]] bool saveGlobalSettings(QSettings& settings, const GlobalSettings& globalSettings);

    [[nodiscard]] bool hasRecord(const QSettings& settings, const QDate& date);
    [[nodiscard]] AttendanceRecord createRecord(const QDate& date, const AttendanceRecord& schedule);
    [[nodiscard]] std::optional<AttendanceRecord>
        loadRecord(const QSettings& settings, const QDate& date, const AttendanceRecord& scheduleFallback);
    [[nodiscard]] bool saveRecord(QSettings& settings, const QDate& date, const AttendanceRecord& record);
    [[nodiscard]] bool removeRecord(QSettings& settings, const QDate& date);
    [[nodiscard]] bool migrateLegacyRecords(QSettings& settings, const AttendanceRecord& currentSchedule);
} // namespace AttendanceSettings
