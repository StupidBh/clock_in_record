#include "CustomCalendarWidget.h"
#include <QSettings>
#include <QStyle>
#include <QApplication>
#include <qDebug>
#include <QAbstractItemModel>
#include <QPainter>

//#include "CustomDateDelegate.h"

CustomCalendarWidget::CustomCalendarWidget(QWidget* parent) : QCalendarWidget(parent), m_tableView(nullptr) {
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
        showContextMenu(QPoint(pos.x(), pos.y()));
    });
    setupEventFilters();
}

void CustomCalendarWidget::setupEventFilters()
{
    m_tableView = this->findChild<QTableView*>();
    if (m_tableView) {
        // 只监听右键，双击用重写的方法处理
        m_tableView->installEventFilter(this);
        //m_tableView->setItemDelegate(new DebugDelegate(m_tableView));
    }
}

void CustomCalendarWidget::paintCell(QPainter* painter, const QRect& rect, const QDate& date) const
{
    __super::paintCell(painter, rect, date);



    if (date == selectedDate())
    {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(0, 145, 255));

        painter->drawRoundedRect(rect.x(), rect.y() + 3, rect.width(), rect.height() - 6, 3, 3);
        painter->setPen(QColor(255, 255, 255));

        painter->drawText(rect, Qt::AlignCenter, QString::number(date.day()));
        painter->restore();
    }
    else if (date == QDate::currentDate())
    {
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



    painter->save();
    QFont font = painter->font();
    QFontMetrics fm(font);
    font.setPointSize(7);
    painter->setFont(font);
    painter->setPen(QPen(Qt::blue));

    QAbstractItemModel* model = m_tableView->model();
    QRect eventRectDown = rect.adjusted(2, rect.height() / 2, -2, -2);
    QRect eventRectUp = rect.adjusted(2, -32, -2, -2);;
    painter->drawText(eventRectUp, Qt::AlignCenter, m_data[date]["arrivalTime"].toString());
    painter->drawText(eventRectDown, Qt::AlignCenter, m_data[date]["departureTime"].toString());
    painter->restore();

}

void CustomCalendarWidget::setCustomData(const QDate& date, const QVariantMap& value)
{
    m_data[date] = value;
    updateCell(date); // 触发paintCell
}


void CustomCalendarWidget::showContextMenu(const QPoint& pos) {
    // 获取点击位置对应的日期
    QDate clickedDate = dateAt(pos);
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

    // 显示菜单并处理选择
    QAction* selectedAction = contextMenu.exec(mapToGlobal(QPoint(pos.x(), pos.y()))); // 显示的时候把判断时加的偏移复位
    if (selectedAction == deleteAction) {
        emit deleteRequested(clickedDate);
    }
}

QDate CustomCalendarWidget::dateAt(const QPoint& pos)
{
    // 这是一个简化的实现，在实际使用中可能需要更精确的计算
    // 使用selectedDate作为近似值
    return getDateFromPosition(QPoint(pos.x(), pos.y() + 50));
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
    
    qDebug() << "Can set data?" <<( model->flags(index) & Qt::ItemIsEditable);
    if (!model->setData(index, 666, Qt::UserRole)) {
        qDebug() << "XXXXXXXXXXXX";
    }
    else {
        qDebug() << "YYYYYYYYYYYYYY";
    }

    return QDate(year, month, day);
}
