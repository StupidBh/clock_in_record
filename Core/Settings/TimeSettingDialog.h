#pragma once
#include "Attendance/AttendanceTypes.h"

#include <QCheckBox>
#include <QDate>
#include <QDialog>
#include <QLabel>
#include <QTimeEdit>

// 时间设置对话框
class TimeSettingDialog : public QDialog {
    Q_OBJECT

public:
    explicit TimeSettingDialog(const QDate& date, QWidget* parent = nullptr);
    AttendanceRecord getRecord() const;

private slots:
    void calculateWorkTime();
    void saveAndClose();

private:
    void setupUI();
    void loadRecord();
    void saveRecord();

    QDate m_date;
    QCheckBox* m_needAverageCalCheckBox;
    QTimeEdit* m_arrivalTimeEdit;
    QTimeEdit* m_departureTimeEdit;
    QLabel* m_resultLabel;
    AttendanceRecord m_globalDefaults;
};
