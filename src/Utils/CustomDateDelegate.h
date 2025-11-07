// #pragma once
//
// #include <QObject>
// #include <QDate>
// #include <qDebug>
// #include <QPainter>
// #include <QStyledItemDelegate>
//
// class CustomDateDelegate : public QStyledItemDelegate {
// public:
//     CustomDateDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) {
//         // 添加一些示例节假日
//         m_holidays.insert(QDate(QDate::currentDate().year(), 10, 1)); // 国庆节
//         m_holidays.insert(QDate(QDate::currentDate().year(), 10, 24)); // 今天
//     }
//
//     void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
//         // --- 修正的关键部分 ---
//         // 使用 Qt::UserRole 获取真实的 QDate 对象
//         QDate date = index.data(Qt::UserRole).toDate();
//         QDate today = QDate::currentDate();
//
//         // 如果日期无效，则不进行任何绘制
//         if (!date.isValid()) {
//             return;
//         }
//
//         // 准备绘制选项
//         QStyleOptionViewItem opt = option;
//         initStyleOption(&opt, index);
//
//         // 绘制背景（处理选中、悬停等状态）
//         painter->fillRect(opt.rect, opt.backgroundBrush);
//
//         painter->save();
//
//         // --- 自定义绘制逻辑 ---
//
//         // 1. 设置字体和颜色
//         QFont font = opt.font;
//         if (m_holidays.contains(date)) {
//             font.setBold(true);
//             painter->setPen(QPen(Qt::green));
//         }
//         else if (date.dayOfWeek() == Qt::Saturday || date.dayOfWeek() == Qt::Sunday) {
//             painter->setPen(QPen(Qt::red));
//         }
//         else {
//             painter->setPen(QPen(opt.palette.color(QPalette::Text)));
//         }
//         painter->setFont(font);
//
//         // 2. 绘制日期数字
//         painter->drawText(opt.rect, Qt::AlignCenter, QString::number(date.day()));
//
//         // 3. 如果是今天，画一个蓝色小圆点
//         if (date == today) {
//             painter->setBrush(QBrush(Qt::blue));
//             painter->setPen(Qt::NoPen);
//             // 在右上角画一个半径为3的圆点
//             QPoint topRight = opt.rect.topRight();
//             painter->drawEllipse(topRight.x() - 8, topRight.y() + 8, 6, 6);
//         }
//
//         painter->restore();
//     }
//
// private:
//     QSet<QDate> m_holidays; // 用于存储节假日
// };
//
//
// class DebugDelegate : public QStyledItemDelegate {
//
// public:
//     DebugDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) {
//     }
//     void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
//         // 我们只打印第一个可见单元格的数据
//         if (index.row() == 0 && index.column() == 0) {
//             qDebug() << "--- Debugging Index (0,0) ---";
//             qDebug() << "Display Role:" << index.data(Qt::DisplayRole) << index.data(Qt::DisplayRole).typeName();
//             qDebug() << "User Role:" << index.data(Qt::UserRole) << index.data(Qt::UserRole).typeName();
//             qDebug() << "UserRole + 1:" << index.data(Qt::UserRole + 1) << index.data(Qt::UserRole + 1).typeName();
//             qDebug() << "UserRole + 2:" << index.data(Qt::UserRole + 2) << index.data(Qt::UserRole + 2).typeName();
//             // ... 等等
//         }
//         // 调用基类绘制，以便我们能看到日历
//         QStyledItemDelegate::paint(painter, option, index);
//     }
// };
