#pragma once
#include "Attendance/AttendanceTypes.h"
#include <QCheckBox>
#include <QDate>
#include <QDoubleSpinBox>
#include <QEvent>
#include <QLabel>
#include <QMainWindow>
#include <QTimeEdit>

class CollapsibleGroupBox;

class CustomCalendarWidget;

// 主窗口
class AttendanceMainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit AttendanceMainWindow(QWidget* parent = nullptr);
    void raiseAndActivate();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onDateClicked(const QDate& date);
    void onMonthChanged();
    void onDeleteRequested(const QDate& date);
    void onGlobalSettingsChanged() const;

private:
    void setupUI();

    void deleteAttendanceRecord(const QDate& date) const;
    void updateCalendarAppearance() const;
    void updateMonthlyStatistics() const;
    void loadGlobalSettings() const;
    void saveGlobalSettings() const;
    void migrateLegacyRecordsToCurrentSchedule() const;
    AttendanceRecord currentGlobalSettings() const;

    CustomCalendarWidget* m_calendar;
    QLabel* m_statsLabel;
    CollapsibleGroupBox* m_globalSettingsGroup;
    QLabel* m_globalSettingsErrorLabel;
    QTimeEdit* m_globalWorkStartEdit;
    QTimeEdit* m_globalWorkEndEdit;
    QTimeEdit* m_globalLunchStartEdit;
    QTimeEdit* m_globalLunchEndEdit;
    QTimeEdit* m_globalDinnerStartEdit;
    QTimeEdit* m_globalDinnerEndEdit;
    QCheckBox* m_mealSubsidyEnabledCheckBox;
    QTimeEdit* m_globalMealSubsidyTimeEdit;
    QCheckBox* m_overtimeOffsetsMissingWorkCheckBox;
    QDoubleSpinBox* m_targetOvertimeHoursSpinBox;
};
