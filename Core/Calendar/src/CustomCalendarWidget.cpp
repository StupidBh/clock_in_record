#include "Calendar/CustomCalendarWidget.h"
#include "Application/Theme.h"
#include "Settings/AttendanceSettings.h"

#include <QContextMenuEvent>
#include <QFontMetrics>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterStateGuard>
#include <QSettings>
#include <QStyle>
#include <QTableView>
#include <QTextCharFormat>

#include <algorithm>
#include <utility>

using namespace Qt::StringLiterals;

CustomCalendarWidget::CustomCalendarWidget(QWidget* parent) :
    QCalendarWidget(parent)
{
    setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);

    m_tableView = findChild<QTableView*>();
    if (m_tableView && m_tableView->viewport()) {
        m_tableView->viewport()->installEventFilter(this);
    }
    updateThemeFormats();

    connect(this, &QCalendarWidget::currentPageChanged, this, [this]() { m_dateRects.clear(); });
}

bool CustomCalendarWidget::eventFilter(QObject* watched, QEvent* event)
{
    if (m_tableView && watched == m_tableView->viewport()) {
        if (event->type() == QEvent::MouseButtonPress) {
            const auto* mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton && selectionMode() == QCalendarWidget::NoSelection) {
                setSelectionMode(QCalendarWidget::SingleSelection);
            }
        }
        else if (event->type() == QEvent::ContextMenu) {
            const auto* contextMenuEvent = static_cast<QContextMenuEvent*>(event);
            showContextMenu(contextMenuEvent->pos());
            return true;
        }
    }

    return QCalendarWidget::eventFilter(watched, event);
}

void CustomCalendarWidget::changeEvent(QEvent* event)
{
    QCalendarWidget::changeEvent(event);
    if (event->type() == QEvent::ApplicationPaletteChange || event->type() == QEvent::PaletteChange) {
        updateThemeFormats();
    }
}

void CustomCalendarWidget::paintCell(QPainter* painter, const QRect& rect, const QDate date) const
{
    m_dateRects[date] = rect;

    QCalendarWidget::paintCell(painter, rect, date);

    const bool isSelected = selectionMode() != QCalendarWidget::NoSelection && date == selectedDate();
    const QPalette calendarPalette = palette();
    const QColor highlight = calendarPalette.color(QPalette::Highlight);
    const QColor highlightedText = calendarPalette.color(QPalette::HighlightedText);

    const auto it = m_attendanceData.constFind(date);
    if (it != m_attendanceData.constEnd()) {
        QPainterStateGuard guard(painter);
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setClipRect(rect.adjusted(1, 1, -1, -1));

        const QTextCharFormat format = dateTextFormat(date);
        QColor background = calendarPalette.color(QPalette::Base);
        if (format.background().style() != Qt::NoBrush) {
            background = format.background().color();
        }

        const QRect cellRect = rect.adjusted(1, 3, -1, -3);
        painter->setPen(Qt::NoPen);
        painter->setBrush(isSelected ? highlight : background);
        painter->drawRoundedRect(cellRect, 3, 3);

        QColor foreground = calendarPalette.color(QPalette::Text);
        if (format.foreground().style() != Qt::NoBrush) {
            foreground = format.foreground().color();
        }
        painter->setPen(isSelected ? highlightedText : foreground);

        QFont dayFont = painter->font();
        dayFont.setWeight(QFont::DemiBold);
        painter->setFont(dayFont);
        const int dayLineHeight = QFontMetrics(dayFont).height();
        const QRect dayRect = cellRect.adjusted(5, 1, -5, 0);
        painter->drawText(dayRect, Qt::AlignLeft | Qt::AlignTop, QString::number(date.day()));

        QFont timeFont = dayFont;
        timeFont.setWeight(QFont::Normal);
        if (timeFont.pointSizeF() > 0.0) {
            timeFont.setPointSizeF(std::max(8.0, timeFont.pointSizeF() * 0.9));
        }
        else if (timeFont.pixelSize() > 0) {
            timeFont.setPixelSize(std::max(11, timeFont.pixelSize() - 1));
        }
        painter->setFont(timeFont);

        const CalendarAttendanceData& attendanceData = it.value();
        const QRect timeRect = cellRect.adjusted(3, dayLineHeight, -3, -2);
        const int halfHeight = timeRect.height() / 2;
        const QRect arrivalRect(timeRect.left(), timeRect.top(), timeRect.width(), halfHeight);
        const QRect departureRect(timeRect.left(), timeRect.top() + halfHeight, timeRect.width(), halfHeight);
        painter->drawText(arrivalRect, Qt::AlignCenter, attendanceData.arrivalTime);
        painter->drawText(departureRect, Qt::AlignCenter, attendanceData.departureTime);

        if (date == QDate::currentDate() && !isSelected) {
            painter->setBrush(Qt::NoBrush);
            painter->setPen(QPen(highlight, 1));
            painter->drawRoundedRect(cellRect.adjusted(1, 1, -1, -1), 2, 2);
        }
        return;
    }

    if (isSelected) {
        QPainterStateGuard guard(painter);
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(Qt::NoPen);
        painter->setBrush(highlight);

        painter->drawRoundedRect(rect.x(), rect.y() + 3, rect.width(), rect.height() - 6, 3, 3);
        painter->setPen(highlightedText);

        painter->drawText(rect, Qt::AlignCenter, QString::number(date.day()));
    }
    else if (date == QDate::currentDate()) {
        QPainterStateGuard guard(painter);
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(Qt::NoPen);
        painter->setBrush(highlight);
        painter->drawRoundedRect(rect.x(), rect.y() + 3, rect.width(), rect.height() - 6, 3, 3);
        painter->setBrush(calendarPalette.color(QPalette::Base));
        painter->drawRoundedRect(rect.x() + 1, rect.y() + 4, rect.width() - 2, rect.height() - 8, 2, 2);
        painter->setPen(highlight);

        painter->drawText(rect, Qt::AlignCenter, QString::number(date.day()));
    }
}

void CustomCalendarWidget::setAttendanceData(const QDate& date, CalendarAttendanceData attendanceData)
{
    m_attendanceData.insert(date, std::move(attendanceData));
    updateCell(date);
}

void CustomCalendarWidget::removeAttendanceData(const QDate& date)
{
    m_attendanceData.remove(date);
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
    const QDate clickedDate = dateFromPosition(pos);
    if (!clickedDate.isValid()) {
        return;
    }

    QSettings settings;
    if (!AttendanceSettings::hasRecord(settings, clickedDate)) {
        return;
    }

    QMenu contextMenu(this);
    QAction* const deleteAction = contextMenu.addAction(u"删除 %1 的记录"_s.arg(clickedDate.toString(u"yyyy-MM-dd"_s)));
    deleteAction->setIcon(style()->standardIcon(QStyle::SP_TrashIcon));

    const QPoint globalPos = m_tableView ? m_tableView->viewport()->mapToGlobal(pos) : mapToGlobal(pos);
    QAction* const selectedAction = contextMenu.exec(globalPos);
    if (selectedAction == deleteAction) {
        emit deleteRequested(clickedDate);
    }
}

QDate CustomCalendarWidget::dateFromPosition(const QPoint& pos) const
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

void CustomCalendarWidget::updateThemeFormats()
{
    QTextCharFormat weekendFormat;
    weekendFormat.setForeground(AttendanceTheme::weekendForeground(palette()));
    setWeekdayTextFormat(Qt::Saturday, weekendFormat);
    setWeekdayTextFormat(Qt::Sunday, weekendFormat);
}
