#pragma once
#include "AttendanceTypes.h"

#include <QCheckBox>
#include <QDate>
#include <QDialog>
#include <QLabel>
#include <QTimeEdit>

class CollapsibleGroupBox;

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
    QTimeEdit* m_workStartTimeEdit;
    QTimeEdit* m_workEndTimeEdit;
    QTimeEdit* m_lunchBreakStartEdit;
    QTimeEdit* m_lunchBreakEndEdit;
    QTimeEdit* m_dinnerBreakStartEdit;
    QTimeEdit* m_dinnerBreakEndEdit;
    QLabel* m_resultLabel;
};
