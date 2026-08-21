#include "Application/AttendanceMainWindow.h"

#include "Application/Theme.h"
#include "Attendance/AttendanceFormatter.h"
#include "Attendance/MonthlyStatisticsCalculator.h"
#include "Attendance/WorkTimeCalculator.h"
#include "Calendar/CustomCalendarWidget.h"
#include "Settings/AttendanceSettings.h"
#include "Settings/TimeSettingDialog.h"
#include "Widgets/CollapsibleGroupBox.h"

#include <QApplication>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLocale>
#include <QMessageBox>
#include <QScrollArea>
#include <QSettings>
#include <QSizePolicy>
#include <QSplitter>
#include <QStringView>
#include <QTextCharFormat>
#include <QTimeEdit>
#include <QVBoxLayout>

#include <array>
#include <vector>

using namespace Qt::StringLiterals;

namespace {
    struct MonthRange
    {
        QDate first;
        QDate last;
    };

    [[nodiscard]] MonthRange monthRange(const int year, const int month)
    {
        const QDate first(year, month, 1);
        return { .first = first, .last = first.addMonths(1).addDays(-1) };
    }

    QTimeEdit* addTimeEditor(QGridLayout& layout, const QStringView label, const int row)
    {
        layout.addWidget(new QLabel(label.toString()), row, 0);
        auto* const editor = new QTimeEdit();
        editor->setDisplayFormat(u"hh:mm"_s);
        layout.addWidget(editor, row, 1);
        return editor;
    }
} // namespace

AttendanceMainWindow::AttendanceMainWindow(QWidget* parent) :
    QMainWindow(parent)
{
    setWindowTitle(u"打卡管理系统"_s);
    setMinimumSize(720, 520);
    resize(980, 700);

    setupUi();
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

void AttendanceMainWindow::changeEvent(QEvent* event)
{
    QMainWindow::changeEvent(event);
    if (event->type() == QEvent::ApplicationPaletteChange && m_calendar) {
        updateCalendarAppearance();
    }
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
    const auto response = QMessageBox::question(this,
                                                u"确认删除"_s,
                                                u"确定要删除 %1 的考勤记录吗？"_s.arg(date.toString(u"yyyy-MM-dd"_s)),
                                                QMessageBox::Yes | QMessageBox::No,
                                                QMessageBox::No);

    if (response == QMessageBox::Yes) {
        deleteAttendanceRecord(date);
    }
}

void AttendanceMainWindow::setupUi()
{
    auto* const centralWidget = new QWidget();
    setCentralWidget(centralWidget);

    auto* const mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(16, 14, 16, 16);
    mainLayout->setSpacing(12);

    auto* const titleLabel = new QLabel(u"考勤日历"_s);
    titleLabel->setObjectName(u"pageTitleLabel"_s);
    titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    titleLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    mainLayout->addWidget(titleLabel);

    auto* const splitter = new QSplitter(Qt::Horizontal);
    splitter->setChildrenCollapsible(false);

    auto* const leftLayout = new QVBoxLayout();
    leftLayout->setContentsMargins(0, 0, 0, 0);

    m_calendar = new CustomCalendarWidget();
    m_calendar->setLocale(QLocale::Chinese);
    m_calendar->setFirstDayOfWeek(Qt::Monday);
    m_calendar->setGridVisible(true);
    leftLayout->addWidget(m_calendar);

    auto* const leftWidget = new QWidget();
    leftWidget->setLayout(leftLayout);
    splitter->addWidget(leftWidget);

    auto* const rightWidget = new QWidget();
    rightWidget->setObjectName(u"inspectorPanel"_s);
    auto* const rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(16, 14, 16, 16);
    rightLayout->setSpacing(12);

    auto* const statsTitleLabel = new QLabel(u"月度统计"_s);
    statsTitleLabel->setObjectName(u"sectionTitleLabel"_s);
    rightLayout->addWidget(statsTitleLabel);

    m_statsLabel = new QLabel(u"请选择月份查看统计"_s);
    m_statsLabel->setObjectName(u"monthlyStatisticsLabel"_s);
    m_statsLabel->setWordWrap(true);
    rightLayout->addWidget(m_statsLabel);

    auto* const separator = new QFrame();
    separator->setObjectName(u"inspectorSeparator"_s);
    separator->setFrameShape(QFrame::HLine);
    rightLayout->addWidget(separator);

    auto* const globalSettingsGroup = new CollapsibleGroupBox(u"全局工作时间设置"_s);
    auto* const globalDetailsLayout = new QVBoxLayout();
    globalDetailsLayout->setContentsMargins(0, 0, 0, 0);
    globalDetailsLayout->setSpacing(10);

    auto* const standardGroup = new QGroupBox(u"标准工时"_s);
    auto* const standardLayout = new QGridLayout(standardGroup);
    standardLayout->setColumnStretch(1, 1);

    m_globalWorkStartEdit = addTimeEditor(*standardLayout, u"上班时间：", 0);
    m_globalWorkEndEdit = addTimeEditor(*standardLayout, u"下班时间：", 1);

    globalDetailsLayout->addWidget(standardGroup);

    auto* const breakGroup = new QGroupBox(u"休息时间"_s);
    auto* const breakLayout = new QGridLayout(breakGroup);
    breakLayout->setColumnStretch(1, 1);

    m_globalLunchStartEdit = addTimeEditor(*breakLayout, u"午餐开始：", 0);
    m_globalLunchEndEdit = addTimeEditor(*breakLayout, u"午餐结束：", 1);
    m_globalDinnerStartEdit = addTimeEditor(*breakLayout, u"晚餐开始：", 2);
    m_globalDinnerEndEdit = addTimeEditor(*breakLayout, u"晚餐结束：", 3);

    globalDetailsLayout->addWidget(breakGroup);

    auto* const mealSubsidyGroup = new QGroupBox(u"餐补"_s);
    auto* const mealSubsidyLayout = new QGridLayout(mealSubsidyGroup);
    mealSubsidyLayout->setColumnStretch(1, 1);
    m_mealSubsidyEnabledCheckBox = new QCheckBox(u"启用餐补统计"_s);
    mealSubsidyLayout->addWidget(m_mealSubsidyEnabledCheckBox, 0, 0, 1, 2);
    m_globalMealSubsidyTimeEdit = addTimeEditor(*mealSubsidyLayout, u"起算时间：", 1);
    globalDetailsLayout->addWidget(mealSubsidyGroup);

    auto* const overtimeTargetGroup = new QGroupBox(u"加班"_s);
    auto* const overtimeTargetLayout = new QGridLayout(overtimeTargetGroup);
    overtimeTargetLayout->setColumnStretch(1, 1);
    overtimeTargetLayout->addWidget(new QLabel(u"日均时长："_s), 0, 0);
    m_targetOvertimeHoursSpinBox = new QDoubleSpinBox();
    m_targetOvertimeHoursSpinBox->setRange(0.0, 24.0);
    m_targetOvertimeHoursSpinBox->setDecimals(1);
    m_targetOvertimeHoursSpinBox->setSingleStep(0.5);
    m_targetOvertimeHoursSpinBox->setSuffix(u" 小时"_s);
    overtimeTargetLayout->addWidget(m_targetOvertimeHoursSpinBox, 0, 1);
    m_overtimeOffsetsMissingWorkCheckBox = new QCheckBox(u"加班抵扣缺少的标准工时"_s);
    m_overtimeOffsetsMissingWorkCheckBox->setObjectName(u"overtimeOffsetsMissingWorkCheckBox"_s);
    overtimeTargetLayout->addWidget(m_overtimeOffsetsMissingWorkCheckBox, 1, 0, 1, 2);
    globalDetailsLayout->addWidget(overtimeTargetGroup);

    m_globalSettingsErrorLabel = new QLabel();
    m_globalSettingsErrorLabel->setObjectName(u"settingsErrorLabel"_s);
    m_globalSettingsErrorLabel->setWordWrap(true);
    m_globalSettingsErrorLabel->setVisible(false);
    globalDetailsLayout->addWidget(m_globalSettingsErrorLabel);

    globalSettingsGroup->setContentLayout(globalDetailsLayout);
    rightLayout->addWidget(globalSettingsGroup);
    rightLayout->addStretch();

    auto* const inspectorScrollArea = new QScrollArea();
    inspectorScrollArea->setObjectName(u"inspectorScrollArea"_s);
    inspectorScrollArea->setWidgetResizable(true);
    inspectorScrollArea->setFrameShape(QFrame::NoFrame);
    inspectorScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    inspectorScrollArea->setMinimumWidth(280);
    inspectorScrollArea->setMaximumWidth(380);
    inspectorScrollArea->setWidget(rightWidget);
    splitter->addWidget(inspectorScrollArea);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({ 640, 300 });

    mainLayout->addWidget(splitter, 1);

    connect(m_calendar, &QCalendarWidget::clicked, this, &AttendanceMainWindow::onDateClicked);
    connect(m_calendar, &QCalendarWidget::currentPageChanged, this, &AttendanceMainWindow::onMonthChanged);
    connect(m_calendar, &CustomCalendarWidget::deleteRequested, this, &AttendanceMainWindow::onDeleteRequested);

    loadGlobalSettings();
    migrateLegacyRecordsToCurrentSchedule();

    const std::array timeEditors {
        m_globalWorkStartEdit,   m_globalWorkEndEdit,   m_globalLunchStartEdit,      m_globalLunchEndEdit,
        m_globalDinnerStartEdit, m_globalDinnerEndEdit, m_globalMealSubsidyTimeEdit,
    };
    for (QTimeEdit* timeEditor : timeEditors) {
        connect(timeEditor, &QTimeEdit::timeChanged, this, &AttendanceMainWindow::onGlobalSettingsChanged);
    }
    connect(m_mealSubsidyEnabledCheckBox, &QCheckBox::toggled, m_globalMealSubsidyTimeEdit, &QTimeEdit::setEnabled);
    connect(m_mealSubsidyEnabledCheckBox, &QCheckBox::toggled, this, &AttendanceMainWindow::onGlobalSettingsChanged);
    connect(m_overtimeOffsetsMissingWorkCheckBox,
            &QCheckBox::toggled,
            this,
            &AttendanceMainWindow::onGlobalSettingsChanged);
    connect(m_targetOvertimeHoursSpinBox,
            &QDoubleSpinBox::valueChanged,
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
    m_calendar->removeAttendanceData(date);

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
    if (!WorkTimeCalculator::hasValidSchedule(currentSchedule())) {
        return;
    }

    QSettings settings;
    AttendanceSettings::GlobalSettings globalSettings;
    globalSettings.schedule = currentSchedule();
    globalSettings.mealSubsidyEnabled = m_mealSubsidyEnabledCheckBox->isChecked();
    globalSettings.overtimeOffsetsMissingWork = m_overtimeOffsetsMissingWorkCheckBox->isChecked();
    globalSettings.targetOvertimeMinutes = qRound(m_targetOvertimeHoursSpinBox->value() * 60.0);
    AttendanceSettings::saveGlobalSettings(settings, globalSettings);
}

void AttendanceMainWindow::onGlobalSettingsChanged()
{
    const bool isValid = WorkTimeCalculator::hasValidSchedule(currentSchedule());
    m_globalSettingsErrorLabel->setVisible(!isValid);
    if (!isValid) {
        m_globalSettingsErrorLabel->setText(u"结束时间必须晚于开始时间，且午餐与晚餐时间不能重叠。"_s);
        return;
    }

    m_globalSettingsErrorLabel->clear();
    saveGlobalSettings();
    updateMonthlyStatistics();
}

AttendanceRecord AttendanceMainWindow::currentSchedule() const
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
    AttendanceSettings::migrateLegacyRecords(settings, currentSchedule());
}

void AttendanceMainWindow::updateCalendarAppearance()
{
    const MonthRange dates = monthRange(m_calendar->yearShown(), m_calendar->monthShown());

    QSettings settings;
    const AttendanceRecord schedule = currentSchedule();
    const QPalette calendarPalette = m_calendar->palette();

    for (QDate date = dates.first; date <= dates.last; date = date.addDays(1)) {
        if (const auto record = AttendanceSettings::loadRecord(settings, date, schedule)) {
            QTextCharFormat format;
            format.setBackground(AttendanceTheme::attendanceBackground(calendarPalette, !record->needAverageCal));
            format.setForeground(AttendanceTheme::attendanceForeground(calendarPalette));
            m_calendar->setDateTextFormat(date, format);
        }
        else {
            m_calendar->setDateTextFormat(date, QTextCharFormat());
            m_calendar->removeAttendanceData(date);
        }
    }
}

void AttendanceMainWindow::updateMonthlyStatistics()
{
    const MonthRange dates = monthRange(m_calendar->yearShown(), m_calendar->monthShown());

    QSettings settings;
    const AttendanceRecord scheduleFallback = currentSchedule();
    const bool mealSubsidyEnabled = m_mealSubsidyEnabledCheckBox->isChecked();
    const bool overtimeOffsetsMissingWork = m_overtimeOffsetsMissingWorkCheckBox->isChecked();

    std::vector<AttendanceRecord> records;
    records.reserve(dates.last.day());
    for (QDate date = dates.first; date <= dates.last; date = date.addDays(1)) {
        if (const auto storedRecord = AttendanceSettings::loadRecord(settings, date, scheduleFallback)) {
            const AttendanceRecord& record = *storedRecord;
            records.push_back(record);
            m_calendar->setAttendanceData(date,
                                          {
                                              .arrivalTime = record.arrivalTime.toString(u"hh:mm"_s),
                                              .departureTime = record.departureTime.toString(u"hh:mm"_s),
                                          });
        }
    }

    const int targetMinutesPerDay = qRound(m_targetOvertimeHoursSpinBox->value() * 60.0);
    const MonthlyStatistics statistics =
        MonthlyStatisticsCalculator::calculate(records, mealSubsidyEnabled, overtimeOffsetsMissingWork);
    m_statsLabel->setText(
        AttendanceFormatter::formatMonthlySummary(dates.first, statistics, targetMinutesPerDay, mealSubsidyEnabled));
}
