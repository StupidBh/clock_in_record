#pragma once
#include <QCalendarWidget>
#include <QDate>
#include <QMenu>
#include <QTableView>

// 自定义日历控件，支持右键菜单
class CustomCalendarWidget : public QCalendarWidget {
    Q_OBJECT

  public:
    explicit CustomCalendarWidget(QWidget* parent = nullptr);
    void paintCell(QPainter* painter, const QRect& rect, QDate date) const override;

    void setCustomData(const QDate& date, const QVariantMap& value);
    void removeCustomData(const QDate& date);
  signals:
    void deleteRequested(const QDate& date);

  private slots:
    void showContextMenu(const QPoint& pos);

  private:
    QDate getDateFromPosition(const QPoint& pos) const;

    QMap<QDate, QVariantMap> m_data;
    QTableView* m_tableView;
};
