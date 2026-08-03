#include "AttendanceMainWindow.h"
#include "Utils/CustomCalendarWidget.h"
#include "Utils/TimeSettingDialog.h"
#include "Utils/CollapsibleGroupBox.h"
#include "Cal/WorkTimeCalculator.h"
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QSplitter>
#include <QLocale>
#include <QTextCharFormat>
#include <QSettings>
#include <QMessageBox>
#include <QSignalBlocker>

AttendanceMainWindow::AttendanceMainWindow(QWidget* parent) :
    QMainWindow(parent)
{
    setWindowTitle(QString("打卡管理系统"));
    setMinimumSize(800, 600);
    resize(900, 680);

    setupUI();
}

void AttendanceMainWindow::raiseAndActivate()
{
    show();
    setWindowState(windowState() & ~Qt::WindowMinimized);
    raise();
    activateWindow();
}

void AttendanceMainWindow::mousePressEvent(QMouseEvent* event)
{
    // 检查点击位置是否在日历区域外
    if (m_calendar) {
        QPoint calendarPos = m_calendar->mapFromGlobal(event->globalPosition().toPoint());
        QRect calendarRect = m_calendar->rect();

        // 如果点击在日历外，重置选择状态
        if (!calendarRect.contains(calendarPos)) {
            int shownYear = m_calendar->yearShown();
            int shownMonth = m_calendar->monthShown();
            QSignalBlocker blocker(m_calendar);
            m_calendar->setSelectedDate(QDate(shownYear, shownMonth, 1).addYears(1));
            m_calendar->setCurrentPage(shownYear, shownMonth);
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
    int ret = QMessageBox::question(this,
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

    // 全局工作时间设置 (默认折叠)
    m_globalSettingsGroup = new CollapsibleGroupBox(QString("全局工作时间设置"));
    QVBoxLayout* globalDetailsLayout = new QVBoxLayout();

    // 标准工作时间
    QGroupBox* standardGroup = new QGroupBox(QString("标准工作时间"));
    QGridLayout* standardLayout = new QGridLayout(standardGroup);

    standardLayout->addWidget(new QLabel(QString("标准上班时间:")), 0, 0);
    m_globalWorkStartEdit = new QTimeEdit();
    m_globalWorkStartEdit->setDisplayFormat("hh:mm");
    standardLayout->addWidget(m_globalWorkStartEdit, 0, 1);

    standardLayout->addWidget(new QLabel(QString("标准下班时间:")), 1, 0);
    m_globalWorkEndEdit = new QTimeEdit();
    m_globalWorkEndEdit->setDisplayFormat("hh:mm");
    standardLayout->addWidget(m_globalWorkEndEdit, 1, 1);

    globalDetailsLayout->addWidget(standardGroup);

    // 休息时间设置
    QGroupBox* breakGroup = new QGroupBox(QString("休息时间设置"));
    QGridLayout* breakLayout = new QGridLayout(breakGroup);

    breakLayout->addWidget(new QLabel(QString("午餐开始时间:")), 0, 0);
    m_globalLunchStartEdit = new QTimeEdit();
    m_globalLunchStartEdit->setDisplayFormat("hh:mm");
    breakLayout->addWidget(m_globalLunchStartEdit, 0, 1);

    breakLayout->addWidget(new QLabel(QString("午餐结束时间:")), 1, 0);
    m_globalLunchEndEdit = new QTimeEdit();
    m_globalLunchEndEdit->setDisplayFormat("hh:mm");
    breakLayout->addWidget(m_globalLunchEndEdit, 1, 1);

    breakLayout->addWidget(new QLabel(QString("晚餐开始时间:")), 2, 0);
    m_globalDinnerStartEdit = new QTimeEdit();
    m_globalDinnerStartEdit->setDisplayFormat("hh:mm");
    breakLayout->addWidget(m_globalDinnerStartEdit, 2, 1);

    breakLayout->addWidget(new QLabel(QString("晚餐结束时间:")), 3, 0);
    m_globalDinnerEndEdit = new QTimeEdit();
    m_globalDinnerEndEdit->setDisplayFormat("hh:mm");
    breakLayout->addWidget(m_globalDinnerEndEdit, 3, 1);

    globalDetailsLayout->addWidget(breakGroup);
    m_globalSettingsGroup->setContentLayout(globalDetailsLayout);
    rightLayout->addWidget(m_globalSettingsGroup);
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

    loadGlobalSettings();

    connect(m_globalWorkStartEdit, &QTimeEdit::timeChanged, this, &AttendanceMainWindow::onGlobalSettingsChanged);
    connect(m_globalWorkEndEdit, &QTimeEdit::timeChanged, this, &AttendanceMainWindow::onGlobalSettingsChanged);
    connect(m_globalLunchStartEdit, &QTimeEdit::timeChanged, this, &AttendanceMainWindow::onGlobalSettingsChanged);
    connect(m_globalLunchEndEdit, &QTimeEdit::timeChanged, this, &AttendanceMainWindow::onGlobalSettingsChanged);
    connect(m_globalDinnerStartEdit, &QTimeEdit::timeChanged, this, &AttendanceMainWindow::onGlobalSettingsChanged);
    connect(m_globalDinnerEndEdit, &QTimeEdit::timeChanged, this, &AttendanceMainWindow::onGlobalSettingsChanged);

    updateCalendarAppearance();
    updateMonthlyStatistics();
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
}

void AttendanceMainWindow::loadGlobalSettings()
{
    AttendanceRecord globalDefaults = loadGlobalTimeDefaults();
    m_globalWorkStartEdit->setTime(globalDefaults.workStartTime);
    m_globalWorkEndEdit->setTime(globalDefaults.workEndTime);
    m_globalLunchStartEdit->setTime(globalDefaults.lunchBreakStart);
    m_globalLunchEndEdit->setTime(globalDefaults.lunchBreakEnd);
    m_globalDinnerStartEdit->setTime(globalDefaults.dinnerBreakStart);
    m_globalDinnerEndEdit->setTime(globalDefaults.dinnerBreakEnd);
}

void AttendanceMainWindow::saveGlobalSettings()
{
    QSettings settings;
    settings.setValue("settings/workStart", m_globalWorkStartEdit->time().toString("hh:mm"));
    settings.setValue("settings/workEnd", m_globalWorkEndEdit->time().toString("hh:mm"));
    settings.setValue("settings/lunchStart", m_globalLunchStartEdit->time().toString("hh:mm"));
    settings.setValue("settings/lunchEnd", m_globalLunchEndEdit->time().toString("hh:mm"));
    settings.setValue("settings/dinnerStart", m_globalDinnerStartEdit->time().toString("hh:mm"));
    settings.setValue("settings/dinnerEnd", m_globalDinnerEndEdit->time().toString("hh:mm"));
}

void AttendanceMainWindow::onGlobalSettingsChanged()
{
    saveGlobalSettings();
    updateMonthlyStatistics();
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
            bool defaultNeedAverage = date.dayOfWeek() != 6 && date.dayOfWeek() != 7;
            if (!settings.value(key + "/needAverageCal", defaultNeedAverage).toBool()) {
                defaultCol = QColor("#acfdea");
            }
            format.setBackground(defaultCol);

            m_calendar->setDateTextFormat(date, format);
        }
        else {
            m_calendar->setDateTextFormat(date, QTextCharFormat());
            m_calendar->removeCustomData(date);
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

    // 直接使用已缓存在 QTimeEdit 部件中的全局时间设置，避免重复读取注册表
    QTime globalWorkStart = m_globalWorkStartEdit->time();
    QTime globalWorkEnd = m_globalWorkEndEdit->time();
    QTime globalLunchStart = m_globalLunchStartEdit->time();
    QTime globalLunchEnd = m_globalLunchEndEdit->time();
    QTime globalDinnerStart = m_globalDinnerStartEdit->time();
    QTime globalDinnerEnd = m_globalDinnerEndEdit->time();

    int workDays = 0;
    int totalOvertimeMinutes = 0;
    int totalLateMinutes = 0;
    int totalEarlyLeaveMinutes = 0;

    QDate date = startDate;
    while (date <= endDate) {
        QString key = date.toString("yyyy-MM-dd");

        if (settings.contains(key + "/arrival")) {
            // 加载记录并计算，record 默认构造已包含所有默认值
            AttendanceRecord record;
            // needAverageCal 默认值与 TimeSettingDialog::loadRecord 保持一致：
            // 周末默认不计入、工作日默认计入
            int dayOfWeek = date.dayOfWeek();
            bool defaultNeedAverage = (dayOfWeek != 6 && dayOfWeek != 7);
            record.needAverageCal = settings.value(key + "/needAverageCal", defaultNeedAverage).toBool();
            record.arrivalTime = QTime::fromString(settings.value(key + "/arrival").toString(), "hh:mm");
            record.departureTime = QTime::fromString(settings.value(key + "/departure").toString(), "hh:mm");

            record.workStartTime = globalWorkStart;
            record.workEndTime = globalWorkEnd;
            record.lunchBreakStart = globalLunchStart;
            record.lunchBreakEnd = globalLunchEnd;
            record.dinnerBreakStart = globalDinnerStart;
            record.dinnerBreakEnd = globalDinnerEnd;

            if (record.needAverageCal) {
                workDays++;
            }

            // 计算工作时间数据
            WorkTimeResult result = WorkTimeCalculator::calculateWorkTimeResult(record);

            if (result.overtimeMinutes > 0) {
                totalOvertimeMinutes += result.overtimeMinutes;
            }
            totalLateMinutes += result.lateMinutes;
            totalEarlyLeaveMinutes += result.earlyLeaveMinutes;

            // tableView model 数据映射
            QVariantMap info;
            info["arrivalTime"] = record.arrivalTime.toString("hh:mm");
            info["departureTime"] = record.departureTime.toString("hh:mm");

            m_calendar->setCustomData(date, info);
        }
        date = date.addDays(1);
    }

    QString stats = QString("统计月份: %1年%2月\n").arg(year).arg(month);
    stats += QString("工作天数: %1天\n").arg(workDays);
    if (workDays > 0) {
        double average_overtime_hours = totalOvertimeMinutes / (60.0 * workDays);
        std::string msg = std::format("均加班时间: {:.3f}\n", average_overtime_hours);
        stats += QString(msg.c_str());

        const double targetRatioPerDay = 2.5;                   // 2.5 小时
        const int targetMinutesPerDay = targetRatioPerDay * 60; // 2.5 小时 × 60

        if (average_overtime_hours < targetRatioPerDay) {
            int lackMinutes = targetMinutesPerDay * workDays - totalOvertimeMinutes;
            int lackHours = lackMinutes / 60;
            int remainMinutes = lackMinutes % 60;

            msg = lackHours != 0 ? std::format("缺加班时间: {}小时{}分钟\n", lackHours, remainMinutes)
                                 : std::format("缺加班时间: {}分钟\n", remainMinutes);
            stats += QString(msg.c_str());
        }
        else if (average_overtime_hours > targetRatioPerDay) {
            int extraMinutes = totalOvertimeMinutes - targetMinutesPerDay * workDays;
            int extraHours = extraMinutes / 60;
            int remainMinutes = extraMinutes % 60;

            msg = extraHours != 0 ? std::format("余加班时间: {}小时{}分钟\n", extraHours, remainMinutes)
                                  : std::format("余加班时间: {}分钟\n", remainMinutes);
            stats += QString(msg.c_str());
        }
    }
    stats += QString("总加班时间: %1小时%2分钟").arg(totalOvertimeMinutes / 60).arg(totalOvertimeMinutes % 60);

    m_statsLabel->setText(stats);
}
