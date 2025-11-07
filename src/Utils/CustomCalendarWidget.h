#ifndef CUSTOMCALENDARWIDGET_H
#define CUSTOMCALENDARWIDGET_H

#include <QCalendarWidget>
#include <QTableView>
#include <QMenu>
#include <QAction>
#include <QDate>

// 自定义日历控件，支持右键菜单
class CustomCalendarWidget : public QCalendarWidget {
    Q_OBJECT
    QMap<QDate, QVariantMap> m_data;

public:
    explicit CustomCalendarWidget(QWidget* parent = nullptr);
    void setupEventFilters();

    void paintCell(QPainter* painter, const QRect& rect, const QDate& date) const;

    void setCustomData(const QDate& date, const QVariantMap& value);
signals:
    void deleteRequested(const QDate& date);

private slots:
    void showContextMenu(const QPoint& pos);

private:
    QDate dateAt(const QPoint& pos);
    QDate getDateFromPosition(const QPoint& pos);

    QTableView* m_tableView;
};

#endif // CUSTOMCALENDARWIDGET_H
