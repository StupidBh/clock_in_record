#include "AttendanceMainWindow.h"
#include "Utils/CustomCalendarWidget.h"
#include "Utils/TimeSettingDialog.h"
#include "Cal/WorkTimeCalculator.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QSplitter>
#include <QLocale>
#include <QTextCharFormat>
#include <QSettings>
#include <QMessageBox>

AttendanceMainWindow::AttendanceMainWindow(QWidget* parent) :
    QMainWindow(parent)
{
    setWindowTitle(QString("打卡管理系统"));
    setMinimumSize(800, 600);
    resize(900, 660);

    setupUI();
    loadAttendanceData();
}

void AttendanceMainWindow::mousePressEvent(QMouseEvent* event)
{
    // 检查点击位置是否在日历区域外
    if (m_calendar) {
        QPoint calendarPos = m_calendar->mapFromGlobal(event->globalPos());
        QRect calendarRect = m_calendar->rect();

        // 如果点击在日历外，重置选择状态
        if (!calendarRect.contains(calendarPos)) {
            // 将选择重置为看不见的日期，并将页面调回当前页面，这样看起来像失去焦点
            m_calendar->setSelectedDate(QDate::currentDate().addDays(365));
            m_calendar->setCurrentPage(QDate::currentDate().year(), QDate::currentDate().month());
        }
    }

    QMainWindow::mousePressEvent(event);
}

void AttendanceMainWindow::onDateClicked(const QDate& date)
{
    TimeSettingDialog dialog(date, this);
    if (dialog.exec() == QDialog::Accepted) {
        updateCalendarAppearance();
        updateMonthlyStatistics();
    }
}

void AttendanceMainWindow::onMonthChanged()
{
    updateCalendarAppearance();
    updateMonthlyStatistics();
}

void AttendanceMainWindow::onDeleteRequested(const QDate& date)
{
    // 确认删除
    int ret = QMessageBox::question(
        this,
        QString("确认删除"),
        QString("确定要删除 %1 的考勤记录吗？").arg(date.toString("yyyy-MM-dd")),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        deleteAttendanceRecord(date);
    }
}

void AttendanceMainWindow::setupUI()
{
    QWidget* centralWidget = new QWidget();
    setCentralWidget(centralWidget);

    QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);

    // 左侧：日历
    QVBoxLayout* leftLayout = new QVBoxLayout();

    QLabel* titleLabel = new QLabel(QString("考勤日历"));
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; padding: 10px;");
    leftLayout->addWidget(titleLabel);

    // 使用自定义日历控件
    m_calendar = new CustomCalendarWidget();
    m_calendar->setLocale(QLocale::Chinese);
    m_calendar->setFirstDayOfWeek(Qt::Monday);
    m_calendar->setGridVisible(true);
    leftLayout->addWidget(m_calendar);

    // 添加使用说明
    QLabel* helpLabel = new QLabel(QString(
        "使用说明：\n• 左键点击日期设置考勤时间\n• 右键点击有记录的日期可删除记录\n• 点击日历外区域可重置选择状态"));
    helpLabel->setStyleSheet(
        "color: #666; font-size: 12px; padding: 10px; background-color: #f5f5f5; border-radius: 5px;");
    helpLabel->setWordWrap(true);
    leftLayout->addWidget(helpLabel);

    // 右侧：统计和管理
    QVBoxLayout* rightLayout = new QVBoxLayout();

    // 月度统计
    QGroupBox* statsGroup = new QGroupBox(QString("月度统计"));
    QVBoxLayout* statsLayout = new QVBoxLayout(statsGroup);
    m_statsLabel = new QLabel(QString("请选择月份查看统计"));
    m_statsLabel->setWordWrap(true);
    m_statsLabel->setStyleSheet("padding: 10px; background-color: #f9f9f9; border-radius: 5px;");
    statsLayout->addWidget(m_statsLabel);
    rightLayout->addWidget(statsGroup);
    rightLayout->addStretch();

    // 使用分割器
    QSplitter* splitter = new QSplitter(Qt::Horizontal);

    QWidget* leftWidget = new QWidget();
    leftWidget->setLayout(leftLayout);

    QWidget* rightWidget = new QWidget();
    rightWidget->setLayout(rightLayout);
    rightWidget->setMaximumWidth(350);

    splitter->addWidget(leftWidget);
    splitter->addWidget(rightWidget);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 1);

    mainLayout->addWidget(splitter);

    // 连接信号
    connect(m_calendar, &QCalendarWidget::clicked, this, &AttendanceMainWindow::onDateClicked);
    connect(m_calendar, &QCalendarWidget::currentPageChanged, this, &AttendanceMainWindow::onMonthChanged);
    connect(m_calendar, &CustomCalendarWidget::deleteRequested, this, &AttendanceMainWindow::onDeleteRequested);

    updateCalendarAppearance();
    updateMonthlyStatistics();
}

void AttendanceMainWindow::loadAttendanceData()
{
    // 数据通过QSettings自动加载
}

void AttendanceMainWindow::deleteAttendanceRecord(const QDate& date)
{
    QSettings settings;
    QString key = date.toString("yyyy-MM-dd");

    // 删除所有相关的设置项
    QStringList keys = { key + "/needAverageCal", key + "/arrival",     key + "/departure",
                         key + "/workStart",      key + "/workEnd",     key + "/lunchStart",
                         key + "/lunchEnd",       key + "/dinnerStart", key + "/dinnerEnd" };

    for (const QString& k : keys) {
        settings.remove(k);
    }

    // 更新界面
    updateCalendarAppearance();
    updateMonthlyStatistics();

    // 显示删除成功消息
    QMessageBox::information(
        this,
        QString("删除成功"),
        QString("已成功删除 %1 的考勤记录").arg(date.toString("yyyy-MM-dd")));
}

void AttendanceMainWindow::updateCalendarAppearance()
{
    int year = m_calendar->yearShown();
    int month = m_calendar->monthShown();
    QDate startDate(year, month, 1);
    QDate endDate = startDate.addMonths(1).addDays(-1);

    QSettings settings;

    QDate date = startDate;
    while (date <= endDate) {
        QString key = date.toString("yyyy-MM-dd");

        if (settings.contains(key + "/arrival")) {
            // 有打卡记录，显示绿色背景
            QTextCharFormat format;
            QColor defaultCol(144, 238, 144); // 浅绿色
            if (settings.contains(key + "/needAverageCal")) {
                if (!settings.value(key + "/needAverageCal").toBool()) {
                    defaultCol = QColor("#acfdea");
                }
            }
            format.setBackground(defaultCol);
            m_calendar->setDateTextFormat(date, format);
        }
        else {
            // 清除格式
            m_calendar->setDateTextFormat(date, QTextCharFormat());
        }
        date = date.addDays(1);
    }
}

void AttendanceMainWindow::updateMonthlyStatistics()
{
    int year = m_calendar->yearShown();
    int month = m_calendar->monthShown();
    QDate startDate(year, month, 1);
    QDate endDate = startDate.addMonths(1).addDays(-1);

    QSettings settings;
    int workDays = 0;
    int totalOvertimeMinutes = 0;
    int totalLateMinutes = 0;
    int totalEarlyLeaveMinutes = 0;

    QDate date = startDate;
    while (date <= endDate) {
        QString key = date.toString("yyyy-MM-dd");

        if (settings.contains(key + "/arrival")) {
            workDays++;

            // 加载记录并计算
            AttendanceRecord record;
            record.needAverageCal = settings.value(key + "/needAverageCal").toBool();
            record.arrivalTime = QTime::fromString(settings.value(key + "/arrival").toString(), "hh:mm");
            record.departureTime = QTime::fromString(settings.value(key + "/departure").toString(), "hh:mm");

            record.workStartTime = QTime::fromString(settings.value(key + "/workStart", "09:00").toString(), "hh:mm");
            record.workEndTime = QTime::fromString(settings.value(key + "/workEnd", "18:00").toString(), "hh:mm");

            record.lunchBreakStart =
                QTime::fromString(settings.value(key + "/lunchStart", "12:30").toString(), "hh:mm");
            record.lunchBreakEnd = QTime::fromString(settings.value(key + "/lunchEnd", "13:30").toString(), "hh:mm");
            record.dinnerBreakStart =
                QTime::fromString(settings.value(key + "/dinnerStart", "18:00").toString(), "hh:mm");
            record.dinnerBreakEnd = QTime::fromString(settings.value(key + "/dinnerEnd", "18:30").toString(), "hh:mm");

            if (!record.needAverageCal) {
                workDays--;
            }

            // 计算工作时间数据
            WorkTimeResult result = WorkTimeCalculator::calculateWorkTimeResult(record);

            if (result.overtimeMinutes > 0) {
                totalOvertimeMinutes += result.overtimeMinutes;
            }
            totalLateMinutes += result.lateMinutes;
            totalEarlyLeaveMinutes += result.earlyLeaveMinutes;
        }
        date = date.addDays(1);
    }

    QString stats = QString("统计月份: %1年%2月\n").arg(year).arg(month);
    stats += QString("工作天数: %1天\n").arg(workDays);
    if (workDays > 0) {
        stats += QString("平均加班时间: %1小时\n").arg(totalOvertimeMinutes / (60.0 * workDays), 0, 'f', 3);
    }
    stats += QString("总加班时间: %1小时%2分钟\n").arg(totalOvertimeMinutes / 60).arg(totalOvertimeMinutes % 60);
    stats += QString("总迟到时间: %1小时%2分钟").arg(totalLateMinutes / 60).arg(totalLateMinutes % 60);
    //stats += QString("总早退时间: %1小时%2分钟\n").arg(totalEarlyLeaveMinutes / 60).arg(totalEarlyLeaveMinutes % 60);

    m_statsLabel->setText(stats);
}
