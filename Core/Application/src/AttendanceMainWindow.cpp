#include "Application/AttendanceMainWindow.h"
#include "Attendance/WorkTimeCalculator.h"
#include "Calendar/CustomCalendarWidget.h"
#include "Settings/AttendanceSettings.h"
#include "Settings/TimeSettingDialog.h"
#include "Widgets/CollapsibleGroupBox.h"
#include <QApplication>
#include <QGridLayout>
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
    resize(900, 680);

    setupUI();
    qApp->installEventFilter(this);
}

void AttendanceMainWindow::raiseAndActivate()
{
    show();
    setWindowState(windowState() & ~Qt::WindowMinimized);
    raise();
    activateWindow();
}

bool AttendanceMainWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonPress && m_calendar) {
        auto* clickedWidget = qobject_cast<QWidget*>(watched);
        if (clickedWidget && clickedWidget != m_calendar && !m_calendar->isAncestorOf(clickedWidget)) {
            m_calendar->clearDateSelection();
        }
    }

    return QMainWindow::eventFilter(watched, event);
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

    QGroupBox* mealSubsidyGroup = new QGroupBox(QString("餐补设置"));
    QGridLayout* mealSubsidyLayout = new QGridLayout(mealSubsidyGroup);
    m_mealSubsidyEnabledCheckBox = new QCheckBox(QString("启用餐补统计"));
    mealSubsidyLayout->addWidget(m_mealSubsidyEnabledCheckBox, 0, 0, 1, 2);
    mealSubsidyLayout->addWidget(new QLabel(QString("餐补起算时间:")), 1, 0);
    m_globalMealSubsidyTimeEdit = new QTimeEdit();
    m_globalMealSubsidyTimeEdit->setDisplayFormat("hh:mm");
    mealSubsidyLayout->addWidget(m_globalMealSubsidyTimeEdit, 1, 1);
    globalDetailsLayout->addWidget(mealSubsidyGroup);

    QGroupBox* overtimeTargetGroup = new QGroupBox(QString("加班设置"));
    QGridLayout* overtimeTargetLayout = new QGridLayout(overtimeTargetGroup);
    overtimeTargetLayout->addWidget(new QLabel(QString("日均加班时长:")), 0, 0);
    m_targetOvertimeHoursSpinBox = new QDoubleSpinBox();
    m_targetOvertimeHoursSpinBox->setRange(0.0, 24.0);
    m_targetOvertimeHoursSpinBox->setDecimals(1);
    m_targetOvertimeHoursSpinBox->setSingleStep(0.5);
    m_targetOvertimeHoursSpinBox->setSuffix(QString(" 小时"));
    overtimeTargetLayout->addWidget(m_targetOvertimeHoursSpinBox, 0, 1);
    m_overtimeOffsetsMissingWorkCheckBox = new QCheckBox(QString("加班抵扣缺少的标准工时"));
    m_overtimeOffsetsMissingWorkCheckBox->setObjectName("overtimeOffsetsMissingWorkCheckBox");
    overtimeTargetLayout->addWidget(m_overtimeOffsetsMissingWorkCheckBox, 1, 0, 1, 2);
    globalDetailsLayout->addWidget(overtimeTargetGroup);

    m_globalSettingsErrorLabel = new QLabel();
    m_globalSettingsErrorLabel->setWordWrap(true);
    m_globalSettingsErrorLabel->setStyleSheet("color: #b00020; padding: 4px;");
    m_globalSettingsErrorLabel->setVisible(false);
    globalDetailsLayout->addWidget(m_globalSettingsErrorLabel);

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
    migrateLegacyRecordsToCurrentSchedule();

    connect(m_globalWorkStartEdit, &QTimeEdit::timeChanged, this, &AttendanceMainWindow::onGlobalSettingsChanged);
    connect(m_globalWorkEndEdit, &QTimeEdit::timeChanged, this, &AttendanceMainWindow::onGlobalSettingsChanged);
    connect(m_globalLunchStartEdit, &QTimeEdit::timeChanged, this, &AttendanceMainWindow::onGlobalSettingsChanged);
    connect(m_globalLunchEndEdit, &QTimeEdit::timeChanged, this, &AttendanceMainWindow::onGlobalSettingsChanged);
    connect(m_globalDinnerStartEdit, &QTimeEdit::timeChanged, this, &AttendanceMainWindow::onGlobalSettingsChanged);
    connect(m_globalDinnerEndEdit, &QTimeEdit::timeChanged, this, &AttendanceMainWindow::onGlobalSettingsChanged);
    connect(m_globalMealSubsidyTimeEdit, &QTimeEdit::timeChanged, this, &AttendanceMainWindow::onGlobalSettingsChanged);
    connect(m_mealSubsidyEnabledCheckBox, &QCheckBox::toggled, m_globalMealSubsidyTimeEdit, &QTimeEdit::setEnabled);
    connect(m_mealSubsidyEnabledCheckBox, &QCheckBox::toggled, this, &AttendanceMainWindow::onGlobalSettingsChanged);
    connect(m_overtimeOffsetsMissingWorkCheckBox,
            &QCheckBox::toggled,
            this,
            &AttendanceMainWindow::onGlobalSettingsChanged);
    connect(m_targetOvertimeHoursSpinBox,
            qOverload<double>(&QDoubleSpinBox::valueChanged),
            this,
            &AttendanceMainWindow::onGlobalSettingsChanged);

    updateCalendarAppearance();
    updateMonthlyStatistics();
}

void AttendanceMainWindow::deleteAttendanceRecord(const QDate& date)
{
    QSettings settings;
    AttendanceSettings::removeRecord(settings, date);

    // The calendar view can include dates from adjacent months, which are outside the monthly refresh range.
    m_calendar->setDateTextFormat(date, QTextCharFormat());
    m_calendar->removeCustomData(date);

    // 更新界面
    updateCalendarAppearance();
    updateMonthlyStatistics();
}

void AttendanceMainWindow::loadGlobalSettings()
{
    QSettings settings;
    const AttendanceSettings::GlobalSettings globalSettings = AttendanceSettings::loadGlobalSettings(settings);
    const AttendanceRecord& globalDefaults = globalSettings.schedule;

    m_globalWorkStartEdit->setTime(globalDefaults.workStartTime);
    m_globalWorkEndEdit->setTime(globalDefaults.workEndTime);
    m_globalLunchStartEdit->setTime(globalDefaults.lunchBreakStart);
    m_globalLunchEndEdit->setTime(globalDefaults.lunchBreakEnd);
    m_globalDinnerStartEdit->setTime(globalDefaults.dinnerBreakStart);
    m_globalDinnerEndEdit->setTime(globalDefaults.dinnerBreakEnd);
    m_mealSubsidyEnabledCheckBox->setChecked(globalSettings.mealSubsidyEnabled);
    m_globalMealSubsidyTimeEdit->setTime(globalDefaults.mealSubsidyTime);
    m_globalMealSubsidyTimeEdit->setEnabled(globalSettings.mealSubsidyEnabled);
    m_overtimeOffsetsMissingWorkCheckBox->setChecked(globalSettings.overtimeOffsetsMissingWork);
    m_targetOvertimeHoursSpinBox->setValue(globalSettings.targetOvertimeMinutes / 60.0);

    if (globalSettings.requiresRepair) {
        saveGlobalSettings();
    }
}

void AttendanceMainWindow::saveGlobalSettings()
{
    if (!WorkTimeCalculator::hasValidSchedule(currentGlobalSettings())) {
        return;
    }

    QSettings settings;
    AttendanceSettings::GlobalSettings globalSettings;
    globalSettings.schedule = currentGlobalSettings();
    globalSettings.mealSubsidyEnabled = m_mealSubsidyEnabledCheckBox->isChecked();
    globalSettings.overtimeOffsetsMissingWork = m_overtimeOffsetsMissingWorkCheckBox->isChecked();
    globalSettings.targetOvertimeMinutes = qRound(m_targetOvertimeHoursSpinBox->value() * 60.0);
    AttendanceSettings::saveGlobalSettings(settings, globalSettings);
}

void AttendanceMainWindow::onGlobalSettingsChanged()
{
    bool isValid = WorkTimeCalculator::hasValidSchedule(currentGlobalSettings());
    m_globalSettingsErrorLabel->setVisible(!isValid);
    if (!isValid) {
        m_globalSettingsErrorLabel->setText(QString("结束时间必须晚于开始时间，且午餐与晚餐时间不能重叠。"));
        return;
    }

    m_globalSettingsErrorLabel->clear();
    saveGlobalSettings();
    updateMonthlyStatistics();
}

AttendanceRecord AttendanceMainWindow::currentGlobalSettings() const
{
    AttendanceRecord record;
    record.workStartTime = m_globalWorkStartEdit->time();
    record.workEndTime = m_globalWorkEndEdit->time();
    record.lunchBreakStart = m_globalLunchStartEdit->time();
    record.lunchBreakEnd = m_globalLunchEndEdit->time();
    record.dinnerBreakStart = m_globalDinnerStartEdit->time();
    record.dinnerBreakEnd = m_globalDinnerEndEdit->time();
    record.mealSubsidyTime = m_globalMealSubsidyTimeEdit->time();
    return record;
}

void AttendanceMainWindow::migrateLegacyRecordsToCurrentSchedule()
{
    QSettings settings;
    AttendanceSettings::migrateLegacyRecords(settings, currentGlobalSettings());
}

void AttendanceMainWindow::updateCalendarAppearance()
{
    int year = m_calendar->yearShown();
    int month = m_calendar->monthShown();
    QDate startDate(year, month, 1);
    QDate endDate = startDate.addMonths(1).addDays(-1);

    QSettings settings;
    const AttendanceRecord schedule = currentGlobalSettings();

    QDate date = startDate;
    while (date <= endDate) {
        if (AttendanceSettings::hasRecord(settings, date)) {
            // 有打卡记录，显示绿色背景
            QTextCharFormat format;
            QColor defaultCol(144, 238, 144); // 浅绿色
            const auto record = AttendanceSettings::loadRecord(settings, date, schedule);
            if (record && !record->needAverageCal) {
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
    bool mealSubsidyEnabled = m_mealSubsidyEnabledCheckBox->isChecked();
    QTime globalMealSubsidyTime = m_globalMealSubsidyTimeEdit->time();

    AttendanceRecord scheduleFallback;
    scheduleFallback.workStartTime = globalWorkStart;
    scheduleFallback.workEndTime = globalWorkEnd;
    scheduleFallback.lunchBreakStart = globalLunchStart;
    scheduleFallback.lunchBreakEnd = globalLunchEnd;
    scheduleFallback.dinnerBreakStart = globalDinnerStart;
    scheduleFallback.dinnerBreakEnd = globalDinnerEnd;
    scheduleFallback.mealSubsidyTime = globalMealSubsidyTime;

    int workDays = 0;
    int totalOvertimeMinutes = 0;
    int totalMissingWorkMinutes = 0;
    int mealSubsidyCount = 0;
    const bool overtimeOffsetsMissingWork = m_overtimeOffsetsMissingWorkCheckBox->isChecked();
    QDate date = startDate;
    while (date <= endDate) {
        if (const auto storedRecord = AttendanceSettings::loadRecord(settings, date, scheduleFallback)) {
            const AttendanceRecord& record = *storedRecord;

            bool hasValidAttendance = WorkTimeCalculator::hasValidAttendanceRange(record);
            if (mealSubsidyEnabled && hasValidAttendance && record.departureTime >= record.mealSubsidyTime) {
                mealSubsidyCount++;
            }

            if (hasValidAttendance && WorkTimeCalculator::hasValidSchedule(record)) {
                WorkTimeResult result = WorkTimeCalculator::calculateWorkTimeResult(record);
                if (record.needAverageCal) {
                    workDays++;
                }
                result = WorkTimeCalculator::applyOvertimeOffset(result, overtimeOffsetsMissingWork);
                totalOvertimeMinutes += result.overtimeMinutes;
                totalMissingWorkMinutes += result.missingWorkMinutes;
            }

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
    const int targetMinutesPerDay = qRound(m_targetOvertimeHoursSpinBox->value() * 60.0);
    const int targetOvertimeMinutes = targetMinutesPerDay * workDays;

    auto formatMinutes = [](int minutes) {
        int absoluteMinutes = minutes < 0 ? -minutes : minutes;
        QString sign = minutes < 0 ? QString("-") : QString();
        return QString("%1%2小时%3分钟").arg(sign).arg(absoluteMinutes / 60).arg(absoluteMinutes % 60);
    };

    if (targetMinutesPerDay == 0) {
        stats += QString("总加班时长: %1\n").arg(formatMinutes(totalOvertimeMinutes));
    }
    else {
        if (workDays > 0) {
            double average_overtime_hours = totalOvertimeMinutes / (60.0 * workDays);
            stats += QString("均加班时间: %1小时\n").arg(average_overtime_hours, 0, 'f', 3);
        }
        if (totalOvertimeMinutes < targetOvertimeMinutes) {
            int lackMinutes = targetOvertimeMinutes - totalOvertimeMinutes;
            stats += QString("缺加班时间: %1\n").arg(formatMinutes(lackMinutes));
        }
        else if (totalOvertimeMinutes > targetOvertimeMinutes) {
            int extraMinutes = totalOvertimeMinutes - targetOvertimeMinutes;
            stats += QString("余加班时间: %1\n").arg(formatMinutes(extraMinutes));
        }
    }
    if (totalMissingWorkMinutes > 0) {
        stats += QString("缺少标准工时: %1\n").arg(formatMinutes(totalMissingWorkMinutes));
    }
    if (mealSubsidyEnabled) {
        stats += QString("餐补次数: %1").arg(mealSubsidyCount);
    }

    m_statsLabel->setText(stats);
}
