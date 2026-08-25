#pragma once
#include "Attendance/AttendanceTypes.h"

#include <QDate>
#include <QMainWindow>
#include <QMap>

class QCheckBox;
class QDoubleSpinBox;
class QLabel;
class QTimer;
class QTimeEdit;
class CustomCalendarWidget;

// 主窗口
class AttendanceMainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit AttendanceMainWindow(QWidget* parent = nullptr);
    ~AttendanceMainWindow() override;

    void raiseAndActivate();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void changeEvent(QEvent* event) override;

private slots:
    void onDateClicked(const QDate& date);
    void onMonthChanged(int year, int month);
    void onDeleteRequested(const QDate& date);
    void onGlobalSettingsChanged();

private:
    void setupUi();

    void deleteAttendanceRecord(const QDate& date);
    void reloadMonthData();
    void updateCalendarAppearance();
    void updateMonthlyStatistics();
    void scheduleMonthlyStatisticsUpdate();
    void loadGlobalSettings();
    void persistGlobalSettings();
    void flushPendingGlobalSettings();
    [[nodiscard]] bool saveGlobalSettings();
    [[nodiscard]] bool migrateLegacyRecordsToCurrentSchedule();
    [[nodiscard]] AttendanceRecord currentSchedule() const;

    CustomCalendarWidget* m_calendar = nullptr;
    QLabel* m_statsPeriodLabel = nullptr;
    QLabel* m_statsWorkDaysValueLabel = nullptr;
    QLabel* m_statsOvertimeLabel = nullptr;
    QLabel* m_statsOvertimeValueLabel = nullptr;
    QLabel* m_statsTargetLabel = nullptr;
    QLabel* m_statsTargetValueLabel = nullptr;
    QLabel* m_statsMissingWorkValueLabel = nullptr;
    QLabel* m_statsMealLabel = nullptr;
    QLabel* m_statsMealValueLabel = nullptr;
    QLabel* m_globalSettingsStatusLabel = nullptr;
    QLabel* m_globalSettingsErrorLabel = nullptr;
    QTimeEdit* m_globalWorkStartEdit = nullptr;
    QTimeEdit* m_globalWorkEndEdit = nullptr;
    QTimeEdit* m_globalLunchStartEdit = nullptr;
    QTimeEdit* m_globalLunchEndEdit = nullptr;
    QTimeEdit* m_globalDinnerStartEdit = nullptr;
    QTimeEdit* m_globalDinnerEndEdit = nullptr;
    QCheckBox* m_mealSubsidyEnabledCheckBox = nullptr;
    QTimeEdit* m_globalMealSubsidyTimeEdit = nullptr;
    QCheckBox* m_overtimeOffsetsMissingWorkCheckBox = nullptr;
    QDoubleSpinBox* m_targetOvertimeHoursSpinBox = nullptr;
    QTimer* m_globalSettingsSaveTimer = nullptr;
    QMap<QDate, AttendanceRecord> m_visibleRecords;
    bool m_monthRefreshPending = false;
    bool m_statisticsRefreshPending = false;
};
