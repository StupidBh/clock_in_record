#pragma once
#include <QDate>
#include <QLabel>
#include <QMainWindow>
#include <QMouseEvent>
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
    void mousePressEvent(QMouseEvent* event) override;

  private slots:
    void onDateClicked(const QDate& date);
    void onMonthChanged();
    void onDeleteRequested(const QDate& date);
    void onGlobalSettingsChanged();

  private:
    void setupUI();

    void deleteAttendanceRecord(const QDate& date);
    void updateCalendarAppearance();
    void updateMonthlyStatistics();
    void loadGlobalSettings();
    void saveGlobalSettings();

    CustomCalendarWidget* m_calendar;
    QLabel* m_statsLabel;
    CollapsibleGroupBox* m_globalSettingsGroup;
    QTimeEdit* m_globalWorkStartEdit;
    QTimeEdit* m_globalWorkEndEdit;
    QTimeEdit* m_globalLunchStartEdit;
    QTimeEdit* m_globalLunchEndEdit;
    QTimeEdit* m_globalDinnerStartEdit;
    QTimeEdit* m_globalDinnerEndEdit;
};
