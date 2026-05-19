#include "CustomCalendarWidget.h"
#include <QSettings>
#include <QStyle>
#include <QApplication>
#include <QDebug>
#include <QAbstractItemModel>
#include <QPainter>

CustomCalendarWidget::CustomCalendarWidget(QWidget* parent) :
    QCalendarWidget(parent),
    m_tableView(nullptr)
{
    setContextMenuPolicy(Qt::CustomContextMenu);
    m_tableView = this->findChild<QTableView*>();
    connect(this, &QWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
        if (m_tableView) {
            showContextMenu(m_tableView->viewport()->mapFrom(this, pos));
        }
    });
}

void CustomCalendarWidget::paintCell(QPainter* painter, const QRect& rect, const QDate& date) const
{
    QCalendarWidget::paintCell(painter, rect, date);

    if (date == selectedDate()) {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(0, 145, 255));

        painter->drawRoundedRect(rect.x(), rect.y() + 3, rect.width(), rect.height() - 6, 3, 3);
        painter->setPen(QColor(255, 255, 255));

        painter->drawText(rect, Qt::AlignCenter, QString::number(date.day()));
        painter->restore();
    }
    else if (date == QDate::currentDate()) {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(0, 161, 255));
        painter->drawRoundedRect(rect.x(), rect.y() + 3, rect.width(), rect.height() - 6, 3, 3);
        painter->setBrush(QColor(255, 255, 255));
        painter->drawRoundedRect(rect.x() + 1, rect.y() + 4, rect.width() - 2, rect.height() - 8, 2, 2);
        painter->setPen(QColor(0, 161, 255));

        painter->drawText(rect, Qt::AlignCenter, QString::number(date.day()));

        painter->restore();
    }

    if (m_data.contains(date)) {
        painter->save();
        QFont font = painter->font();
        font.setPointSize(7);
        painter->setFont(font);
        painter->setPen(QPen(Qt::blue));

        QRect eventRectDown = rect.adjusted(2, rect.height() / 2, -2, -2);
        QRect eventRectUp = rect.adjusted(2, -32, -2, -2);
        painter->drawText(eventRectUp, Qt::AlignCenter, m_data[date]["arrivalTime"].toString());
        painter->drawText(eventRectDown, Qt::AlignCenter, m_data[date]["departureTime"].toString());
        painter->restore();
    }
}

void CustomCalendarWidget::setCustomData(const QDate& date, const QVariantMap& value)
{
    m_data[date] = value;
    updateCell(date); // 触发paintCell
}

void CustomCalendarWidget::removeCustomData(const QDate& date)
{
    m_data.remove(date);
    updateCell(date);
}

void CustomCalendarWidget::showContextMenu(const QPoint& pos)
{
    QDate clickedDate = getDateFromPosition(pos);
    if (!clickedDate.isValid()) {
        return;
    }

    // 检查该日期是否有记录
    QSettings settings;
    QString key = clickedDate.toString("yyyy-MM-dd");
    if (!settings.contains(key + "/arrival")) {
        return; // 没有记录，不显示菜单
    }

    // 创建右键菜单
    QMenu contextMenu(this);
    QAction* deleteAction = contextMenu.addAction(QString("删除 %1 的记录").arg(clickedDate.toString("yyyy-MM-dd")));
    deleteAction->setIcon(style()->standardIcon(QStyle::SP_TrashIcon));

    QPoint globalPos = m_tableView ? m_tableView->viewport()->mapToGlobal(pos) : mapToGlobal(pos);
    QAction* selectedAction = contextMenu.exec(globalPos);
    if (selectedAction == deleteAction) {
        emit deleteRequested(clickedDate);
    }
}

QDate CustomCalendarWidget::getDateFromPosition(const QPoint& pos)
{
    if (!m_tableView) {
        return QDate();
    }

    QModelIndex index = m_tableView->indexAt(pos);
    if (!index.isValid()) {
        return QDate();
    }

    // 获取模型
    QAbstractItemModel* model = m_tableView->model();
    if (!model) {
        return QDate();
    }

    int year = yearShown();
    int month = monthShown();
    int day = model->data(index, Qt::DisplayRole).toInt();

    return QDate(year, month, day);
}
