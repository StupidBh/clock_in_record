#include "Calendar/CustomCalendarWidget.h"
#include "Settings/AttendanceSettings.h"
#include <QSettings>
#include <QStyle>
#include <QApplication>
#include <QMouseEvent>
#include <QPainter>

CustomCalendarWidget::CustomCalendarWidget(QWidget* parent) :
    QCalendarWidget(parent),
    m_tableView(nullptr)
{
    m_tableView = this->findChild<QTableView*>();
    if (m_tableView && m_tableView->viewport()) {
        m_tableView->viewport()->installEventFilter(this);
    }

    connect(this, &QCalendarWidget::currentPageChanged, this, [this]() { m_dateRects.clear(); });
}

bool CustomCalendarWidget::eventFilter(QObject* watched, QEvent* event)
{
    if (m_tableView && watched == m_tableView->viewport()) {
        if (event->type() == QEvent::MouseButtonPress) {
            auto* mouseEvent = dynamic_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton && selectionMode() == QCalendarWidget::NoSelection) {
                setSelectionMode(QCalendarWidget::SingleSelection);
            }
        }
        else if (event->type() == QEvent::ContextMenu) {
            auto* contextMenuEvent = dynamic_cast<QContextMenuEvent*>(event);
            showContextMenu(contextMenuEvent->pos());
            return true;
        }
    }

    return QCalendarWidget::eventFilter(watched, event);
}

void CustomCalendarWidget::paintCell(QPainter* painter, const QRect& rect, const QDate date) const
{
    m_dateRects[date] = rect;

    QCalendarWidget::paintCell(painter, rect, date);

    const bool isSelected = selectionMode() != QCalendarWidget::NoSelection && date == selectedDate();
    if (isSelected) {
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

    auto it = m_data.constFind(date);
    if (it != m_data.constEnd()) {
        painter->save();
        QFont font = painter->font();
        font.setPointSize(7);
        painter->setFont(font);
        painter->setPen(isSelected ? QPen(Qt::white) : QPen(QColor(0, 70, 170)));

        const QVariantMap& info = it.value();
        QRect contentRect = rect.adjusted(2, 2, -2, -2);
        int halfHeight = contentRect.height() / 2;
        QRect arrivalRect(contentRect.left(), contentRect.top(), contentRect.width(), halfHeight);
        QRect departureRect(contentRect.left(), contentRect.top() + halfHeight, contentRect.width(), halfHeight);
        painter->drawText(arrivalRect, Qt::AlignCenter, info["arrivalTime"].toString());
        painter->drawText(departureRect, Qt::AlignCenter, info["departureTime"].toString());
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

void CustomCalendarWidget::clearDateSelection()
{
    const QDate previouslySelectedDate = selectedDate();
    setSelectionMode(QCalendarWidget::NoSelection);
    updateCell(previouslySelectedDate);
}

void CustomCalendarWidget::showContextMenu(const QPoint& pos)
{
    QDate clickedDate = getDateFromPosition(pos);
    if (!clickedDate.isValid()) {
        return;
    }

    // 检查该日期是否有记录
    QSettings settings;
    if (!AttendanceSettings::hasRecord(settings, clickedDate)) {
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

QDate CustomCalendarWidget::getDateFromPosition(const QPoint& pos) const
{
    // paintCell 已经拿到了 QCalendarWidget 最终用于绘制每个日期的真实 rect。
    // 直接用这些 rect 做命中测试，避免依赖内部 QTableView 的行列、表头和坐标转换细节。
    for (auto it = m_dateRects.constBegin(); it != m_dateRects.constEnd(); ++it) {
        if (it.value().contains(pos)) {
            return it.key();
        }
    }

    return { };
}
