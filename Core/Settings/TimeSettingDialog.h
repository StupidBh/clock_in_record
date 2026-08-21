#pragma once
#include "Attendance/AttendanceTypes.h"

#include <QDate>
#include <QDialog>

class QCheckBox;
class QLabel;
class QTimeEdit;

// 时间设置对话框
class TimeSettingDialog final : public QDialog {
    Q_OBJECT

public:
    explicit TimeSettingDialog(const QDate& date, QWidget* parent = nullptr);
    [[nodiscard]] AttendanceRecord record() const;

private slots:
    void calculateWorkTime() const;
    void saveAndClose();

private:
    void setupUi();
    void loadRecord();
    void saveRecord() const;

    QDate m_date;
    QCheckBox* m_needAverageCalCheckBox = nullptr;
    QTimeEdit* m_arrivalTimeEdit = nullptr;
    QTimeEdit* m_departureTimeEdit = nullptr;
    QLabel* m_resultLabel = nullptr;
    AttendanceRecord m_globalDefaults;
    bool m_overtimeOffsetsMissingWork = false;
};
