#pragma once

#include <QCalendarWidget>
#include <QDate>
#include <QMap>
#include <QRect>
#include <QString>

class QPainter;
class QTableView;

struct CalendarAttendanceData
{
    QString arrivalTime;
    QString departureTime;
};

// 自定义日历控件，支持右键菜单
class CustomCalendarWidget final : public QCalendarWidget {
    Q_OBJECT

public:
    explicit CustomCalendarWidget(QWidget* parent = nullptr);

    void setAttendanceData(const QDate& date, CalendarAttendanceData attendanceData);
    void removeAttendanceData(const QDate& date);
    void clearDateSelection();

protected:
    void paintCell(QPainter* painter, const QRect& rect, QDate date) const override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    void changeEvent(QEvent* event) override;

signals:
    void deleteRequested(const QDate& date);

private:
    void showContextMenu(const QPoint& pos);
    [[nodiscard]] QDate dateFromPosition(const QPoint& pos) const;
    void updateThemeFormats();

    QMap<QDate, CalendarAttendanceData> m_attendanceData;
    mutable QMap<QDate, QRect> m_dateRects;
    QTableView* m_tableView = nullptr;
};
