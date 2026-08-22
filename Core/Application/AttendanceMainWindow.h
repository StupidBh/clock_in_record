#pragma once

#include "Attendance/AttendanceTypes.h"

#include <QMainWindow>

class QCheckBox;
class QDate;
class QDoubleSpinBox;
class QLabel;
class QTimeEdit;
class CustomCalendarWidget;

// 主窗口
class AttendanceMainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit AttendanceMainWindow(QWidget* parent = nullptr);
    void raiseAndActivate();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void changeEvent(QEvent* event) override;

private slots:
    void onDateClicked(const QDate& date);
    void onMonthChanged();
    void onDeleteRequested(const QDate& date);
    void onGlobalSettingsChanged();

private:
    void setupUi();

    void deleteAttendanceRecord(const QDate& date);
    void updateCalendarAppearance();
    void updateMonthlyStatistics();
    void loadGlobalSettings();
    [[nodiscard]] bool saveGlobalSettings();
    [[nodiscard]] bool migrateLegacyRecordsToCurrentSchedule();
    [[nodiscard]] AttendanceRecord currentSchedule() const;

    CustomCalendarWidget* m_calendar = nullptr;
    QLabel* m_statsLabel = nullptr;
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
};
