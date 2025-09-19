#ifndef CUSTOMCALENDARWIDGET_H
#define CUSTOMCALENDARWIDGET_H

#include <QCalendarWidget>
#include <QTableView>
#include <QMenu>
#include <QAction>
#include <QDate>

// �Զ��������ؼ���֧���Ҽ��˵�
class CustomCalendarWidget : public QCalendarWidget {
    Q_OBJECT

public:
    explicit CustomCalendarWidget(QWidget* parent = nullptr);
    void setupEventFilters();

signals:
    void deleteRequested(const QDate& date);

private slots:
    void showContextMenu(const QPoint& pos);

private:
    QDate dateAt(const QPoint& pos);
    QDate getDateFromPosition(const QPoint& pos);
    QDate calculateDateFromRowCol(int row, int col);

    QTableView* m_tableView;
};

#endif // CUSTOMCALENDARWIDGET_H