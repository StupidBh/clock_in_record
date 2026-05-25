#pragma once
#include <QDate>
#include <QLabel>
#include <QMainWindow>
#include <QMouseEvent>

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

  private:
    void setupUI();

    void deleteAttendanceRecord(const QDate& date);
    void updateCalendarAppearance();
    void updateMonthlyStatistics();

    CustomCalendarWidget* m_calendar;
    QLabel* m_statsLabel;
};
