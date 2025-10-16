#include "CustomCalendarWidget.h"
#include <QSettings>
#include <QStyle>
#include <QApplication>
#include <qDebug>
#include <QAbstractItemModel>

CustomCalendarWidget::CustomCalendarWidget(QWidget* parent) : QCalendarWidget(parent), m_tableView(nullptr) {
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested, this, [this](const QPoint& pos){
        showContextMenu(QPoint(pos.x(), pos.y() - 70));  // 实际点击时垂直方向存在偏移
        });
    setupEventFilters();
}

void CustomCalendarWidget::setupEventFilters() {
    m_tableView = this->findChild<QTableView*>();
    if (m_tableView) {
        // 只监听右键，双击用重写的方法处理
        m_tableView->installEventFilter(this);
    }
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
    QAction* selectedAction = contextMenu.exec(mapToGlobal(QPoint(pos.x(),pos.y()+70))); // 显示的时候把判断时加的偏移复位
    if (selectedAction == deleteAction) {
        emit deleteRequested(clickedDate);
    }
}

QDate CustomCalendarWidget::dateAt(const QPoint& pos) {
    // 这是一个简化的实现，在实际使用中可能需要更精确的计算
    // 使用selectedDate作为近似值
    return getDateFromPosition(QPoint(pos.x(), pos.y() + 50));
}

QDate CustomCalendarWidget::getDateFromPosition(const QPoint& pos) {
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

    // 尝试不同的角色获取日期数据
    QVariant dateData;

    // 尝试 Qt::UserRole
    dateData = model->data(index, Qt::UserRole);
    if (dateData.canConvert<QDate>()) {
        return dateData.toDate();
    }

    // 尝试 Qt::UserRole + 1
    dateData = model->data(index, Qt::UserRole + 1);
    if (dateData.canConvert<QDate>()) {
        return dateData.toDate();
    }

    // 如果都获取不到，使用改进的计算方法
    return calculateDateFromRowCol(index.row(), index.column());
}

QDate CustomCalendarWidget::calculateDateFromRowCol(int row, int col) {
    // 获取当前显示的年月
    int year = yearShown();
    int month = monthShown();
    QDate firstDay(year, month, 1);

    // 获取日历的第一天设置
    Qt::DayOfWeek startDay = firstDayOfWeek();

    // 计算第一天在表格中的位置
    int firstDayColumn;
    if (startDay == Qt::Sunday) {
        firstDayColumn = firstDay.dayOfWeek() % 7; // Sunday = 0
    }
    else {
        firstDayColumn = firstDay.dayOfWeek() - 1; // Monday = 0
    }

    // 计算当前位置对应的天数
    int totalCells = (row - 1) * 7 + col - 1;
    int dayNumber = totalCells - firstDayColumn + 1;

    if (dayNumber <= 0) {
        // 上个月的日期
        QDate prevMonth = firstDay.addMonths(-1);
        return QDate(prevMonth.year(), prevMonth.month(),
            prevMonth.daysInMonth() + dayNumber);
    }
    else if (dayNumber > firstDay.daysInMonth()) {
        // 下个月的日期
        QDate nextMonth = firstDay.addMonths(1);
        return QDate(nextMonth.year(), nextMonth.month(),
            dayNumber - firstDay.daysInMonth());
    }
    else {
        // 当前月的日期
        return QDate(year, month, dayNumber);
    }
}
