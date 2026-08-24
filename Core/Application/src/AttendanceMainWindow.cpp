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
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSettings>
#include <QSizePolicy>
#include <QSplitter>
#include <QStringView>
#include <QStyle>
#include <QTextCharFormat>
#include <QTimeEdit>
#include <QVariant>
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

    struct StatisticRow
    {
        QLabel* label;
        QLabel* value;
    };

    [[nodiscard]] StatisticRow addStatisticRow(QGridLayout& layout, const QStringView text, const int row)
    {
        auto* const label = new QLabel(text.toString());
        label->setProperty("statisticsRole", u"metric"_s);
        auto* const value = new QLabel();
        value->setProperty("statisticsRole", u"value"_s);
        value->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        layout.addWidget(label, row, 0);
        layout.addWidget(value, row, 1);
        return { .label = label, .value = value };
    }

    struct SettingsSection
    {
        QWidget* widget;
        QGridLayout* layout;
    };

    [[nodiscard]] SettingsSection createSettingsSection(const QStringView title)
    {
        auto* const widget = new QWidget();
        auto* const layout = new QGridLayout(widget);
        layout->setContentsMargins(0, 4, 0, 6);
        layout->setHorizontalSpacing(10);
        layout->setVerticalSpacing(8);
        layout->setColumnStretch(1, 1);

        auto* const titleLabel = new QLabel(title.toString());
        titleLabel->setObjectName(u"formSectionLabel"_s);
        layout->addWidget(titleLabel, 0, 0, 1, 2);
        return { .widget = widget, .layout = layout };
    }

    QTimeEdit* addTimeEditor(QGridLayout& layout, const QStringView label, const int row)
    {
        layout.addWidget(new QLabel(label.toString()), row, 0);
        auto* const editor = new QTimeEdit();
        editor->setDisplayFormat(u"hh:mm"_s);
        layout.addWidget(editor, row, 1);
        return editor;
    }

    void setStyleProperty(QWidget& widget, const char* const name, const QVariant& value)
    {
        if (widget.property(name) == value) {
            return;
        }

        widget.setProperty(name, value);
        widget.style()->unpolish(&widget);
        widget.style()->polish(&widget);
        widget.update();
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

    auto* const titleLayout = new QHBoxLayout();
    titleLayout->setContentsMargins(0, 0, 0, 0);
    auto* const titleLabel = new QLabel(u"考勤日历"_s);
    titleLabel->setObjectName(u"pageTitleLabel"_s);
    titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    titleLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();
    auto* const todayButton = new QPushButton(u"今天"_s);
    todayButton->setObjectName(u"todayButton"_s);
    todayButton->setToolTip(u"返回并选中今天"_s);
    titleLayout->addWidget(todayButton);
    mainLayout->addLayout(titleLayout);

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

    auto* const statsHeaderLayout = new QHBoxLayout();
    statsHeaderLayout->setContentsMargins(0, 0, 0, 0);
    auto* const statsTitleLabel = new QLabel(u"月度统计"_s);
    statsTitleLabel->setObjectName(u"sectionTitleLabel"_s);
    statsHeaderLayout->addWidget(statsTitleLabel);
    statsHeaderLayout->addStretch();
    m_statsPeriodLabel = new QLabel();
    m_statsPeriodLabel->setObjectName(u"statisticsPeriodLabel"_s);
    statsHeaderLayout->addWidget(m_statsPeriodLabel);
    rightLayout->addLayout(statsHeaderLayout);

    auto* const statisticsWidget = new QWidget();
    statisticsWidget->setObjectName(u"statisticsGrid"_s);
    auto* const statisticsLayout = new QGridLayout(statisticsWidget);
    statisticsLayout->setContentsMargins(0, 2, 0, 4);
    statisticsLayout->setHorizontalSpacing(16);
    statisticsLayout->setVerticalSpacing(8);
    statisticsLayout->setColumnStretch(1, 1);

    m_statsWorkDaysValueLabel = addStatisticRow(*statisticsLayout, u"工作日"_s, 0).value;
    m_statsWorkDaysValueLabel->setObjectName(u"statsWorkDaysValueLabel"_s);
    const StatisticRow overtimeRow = addStatisticRow(*statisticsLayout, u"日均加班"_s, 1);
    m_statsOvertimeLabel = overtimeRow.label;
    m_statsOvertimeValueLabel = overtimeRow.value;
    m_statsOvertimeValueLabel->setObjectName(u"statsOvertimeValueLabel"_s);
    const StatisticRow targetRow = addStatisticRow(*statisticsLayout, u"距目标"_s, 2);
    m_statsTargetLabel = targetRow.label;
    m_statsTargetValueLabel = targetRow.value;
    m_statsTargetValueLabel->setObjectName(u"statsTargetValueLabel"_s);
    m_statsMissingWorkValueLabel = addStatisticRow(*statisticsLayout, u"工时缺口"_s, 3).value;
    m_statsMissingWorkValueLabel->setObjectName(u"statsMissingWorkValueLabel"_s);
    const StatisticRow mealRow = addStatisticRow(*statisticsLayout, u"餐补"_s, 4);
    m_statsMealLabel = mealRow.label;
    m_statsMealValueLabel = mealRow.value;
    m_statsMealValueLabel->setObjectName(u"statsMealValueLabel"_s);
    rightLayout->addWidget(statisticsWidget);

    auto* const separator = new QFrame();
    separator->setObjectName(u"inspectorSeparator"_s);
    separator->setFrameShape(QFrame::HLine);
    rightLayout->addWidget(separator);

    auto* const globalSettingsGroup = new CollapsibleGroupBox(u"全局工作时间设置"_s);
    globalSettingsGroup->setObjectName(u"globalSettingsGroup"_s);

    auto* const globalDetailsWidget = new QWidget();
    auto* const globalDetailsLayout = new QVBoxLayout(globalDetailsWidget);
    globalDetailsLayout->setContentsMargins(0, 0, 0, 0);
    globalDetailsLayout->setSpacing(10);

    const SettingsSection standardSection = createSettingsSection(u"标准工时"_s);
    m_globalWorkStartEdit = addTimeEditor(*standardSection.layout, u"上班时间"_s, 1);
    m_globalWorkEndEdit = addTimeEditor(*standardSection.layout, u"下班时间"_s, 2);
    globalDetailsLayout->addWidget(standardSection.widget);

    const SettingsSection breakSection = createSettingsSection(u"休息时间"_s);
    m_globalLunchStartEdit = addTimeEditor(*breakSection.layout, u"午餐开始"_s, 1);
    m_globalLunchEndEdit = addTimeEditor(*breakSection.layout, u"午餐结束"_s, 2);
    m_globalDinnerStartEdit = addTimeEditor(*breakSection.layout, u"晚餐开始"_s, 3);
    m_globalDinnerEndEdit = addTimeEditor(*breakSection.layout, u"晚餐结束"_s, 4);
    globalDetailsLayout->addWidget(breakSection.widget);

    const SettingsSection mealSubsidySection = createSettingsSection(u"餐补"_s);
    m_mealSubsidyEnabledCheckBox = new QCheckBox(u"启用餐补统计"_s);
    mealSubsidySection.layout->addWidget(m_mealSubsidyEnabledCheckBox, 1, 0, 1, 2);
    m_globalMealSubsidyTimeEdit = addTimeEditor(*mealSubsidySection.layout, u"起算时间"_s, 2);
    globalDetailsLayout->addWidget(mealSubsidySection.widget);

    const SettingsSection overtimeSection = createSettingsSection(u"加班"_s);
    overtimeSection.layout->addWidget(new QLabel(u"日均目标"_s), 1, 0);
    m_targetOvertimeHoursSpinBox = new QDoubleSpinBox();
    m_targetOvertimeHoursSpinBox->setRange(0.0, 24.0);
    m_targetOvertimeHoursSpinBox->setDecimals(1);
    m_targetOvertimeHoursSpinBox->setSingleStep(0.5);
    m_targetOvertimeHoursSpinBox->setSuffix(u" 小时"_s);
    overtimeSection.layout->addWidget(m_targetOvertimeHoursSpinBox, 1, 1);
    m_overtimeOffsetsMissingWorkCheckBox = new QCheckBox(u"加班抵扣缺少的标准工时"_s);
    m_overtimeOffsetsMissingWorkCheckBox->setObjectName(u"overtimeOffsetsMissingWorkCheckBox"_s);
    overtimeSection.layout->addWidget(m_overtimeOffsetsMissingWorkCheckBox, 2, 0, 1, 2);
    globalDetailsLayout->addWidget(overtimeSection.widget);

    m_globalSettingsStatusLabel = new QLabel(u"更改仅影响新记录"_s);
    m_globalSettingsStatusLabel->setObjectName(u"settingsStatusLabel"_s);
    globalDetailsLayout->addWidget(m_globalSettingsStatusLabel);

    m_globalSettingsErrorLabel = new QLabel();
    m_globalSettingsErrorLabel->setObjectName(u"settingsErrorLabel"_s);
    m_globalSettingsErrorLabel->setWordWrap(true);
    m_globalSettingsErrorLabel->setVisible(false);
    globalDetailsLayout->addWidget(m_globalSettingsErrorLabel);

    auto* const globalSettingsScrollArea = new QScrollArea();
    globalSettingsScrollArea->setObjectName(u"globalSettingsScrollArea"_s);
    globalSettingsScrollArea->setWidgetResizable(true);
    globalSettingsScrollArea->setFrameShape(QFrame::NoFrame);
    globalSettingsScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    globalSettingsScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    globalSettingsScrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Ignored);
    globalSettingsScrollArea->setMinimumHeight(120);
    globalSettingsScrollArea->setWidget(globalDetailsWidget);

    auto* const globalSettingsContentLayout = new QVBoxLayout();
    globalSettingsContentLayout->setContentsMargins(0, 0, 0, 0);
    globalSettingsContentLayout->addWidget(globalSettingsScrollArea);
    globalSettingsGroup->setContentLayout(globalSettingsContentLayout);
    rightLayout->addWidget(globalSettingsGroup, 1);
    rightLayout->setAlignment(globalSettingsGroup, Qt::AlignTop);

    auto* const inspectorScrollArea = new QScrollArea();
    inspectorScrollArea->setObjectName(u"inspectorScrollArea"_s);
    inspectorScrollArea->setWidgetResizable(true);
    inspectorScrollArea->setFrameShape(QFrame::NoFrame);
    inspectorScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    inspectorScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
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
    connect(todayButton, &QPushButton::clicked, this, [this]() {
        m_calendar->setSelectionMode(QCalendarWidget::SingleSelection);
        m_calendar->setSelectedDate(QDate::currentDate());
    });

    loadGlobalSettings();
    if (!migrateLegacyRecordsToCurrentSchedule()) {
        m_globalSettingsStatusLabel->setVisible(false);
        m_globalSettingsErrorLabel->setVisible(true);
        m_globalSettingsErrorLabel->setText(u"历史考勤记录无法完成迁移，请检查设置存储权限。"_s);
    }

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
    if (!AttendanceSettings::removeRecord(settings, date)) {
        QMessageBox::warning(this, u"删除失败"_s, u"考勤记录无法删除，请检查设置存储权限。"_s);
        return;
    }

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
        if (!saveGlobalSettings()) {
            m_globalSettingsStatusLabel->setVisible(false);
            m_globalSettingsErrorLabel->setVisible(true);
            m_globalSettingsErrorLabel->setText(u"全局设置无法保存，请检查设置存储权限。"_s);
        }
    }
}

bool AttendanceMainWindow::saveGlobalSettings()
{
    if (!WorkTimeCalculator::hasValidSchedule(currentSchedule())) {
        return false;
    }

    QSettings settings;
    AttendanceSettings::GlobalSettings globalSettings;
    globalSettings.schedule = currentSchedule();
    globalSettings.mealSubsidyEnabled = m_mealSubsidyEnabledCheckBox->isChecked();
    globalSettings.overtimeOffsetsMissingWork = m_overtimeOffsetsMissingWorkCheckBox->isChecked();
    globalSettings.targetOvertimeMinutes = qRound(m_targetOvertimeHoursSpinBox->value() * 60.0);
    return AttendanceSettings::saveGlobalSettings(settings, globalSettings);
}

void AttendanceMainWindow::onGlobalSettingsChanged()
{
    const AttendanceRecord schedule = currentSchedule();
    const auto isValidRange = [](const QTime& start, const QTime& end) {
        return start.isValid() && end.isValid() && start < end;
    };
    const bool workRangeValid = isValidRange(schedule.workStartTime, schedule.workEndTime);
    const bool lunchRangeValid = isValidRange(schedule.lunchBreakStart, schedule.lunchBreakEnd);
    const bool dinnerRangeValid = isValidRange(schedule.dinnerBreakStart, schedule.dinnerBreakEnd);
    const bool breaksOverlap =
        schedule.lunchBreakStart < schedule.dinnerBreakEnd && schedule.dinnerBreakStart < schedule.lunchBreakEnd;

    setStyleProperty(*m_globalWorkStartEdit, "validationError", !workRangeValid);
    setStyleProperty(*m_globalWorkEndEdit, "validationError", !workRangeValid);
    setStyleProperty(*m_globalLunchStartEdit, "validationError", !lunchRangeValid || breaksOverlap);
    setStyleProperty(*m_globalLunchEndEdit, "validationError", !lunchRangeValid || breaksOverlap);
    setStyleProperty(*m_globalDinnerStartEdit, "validationError", !dinnerRangeValid || breaksOverlap);
    setStyleProperty(*m_globalDinnerEndEdit, "validationError", !dinnerRangeValid || breaksOverlap);

    const bool isValid = WorkTimeCalculator::hasValidSchedule(schedule);
    m_globalSettingsErrorLabel->setVisible(!isValid);
    m_globalSettingsStatusLabel->setVisible(isValid);
    if (!isValid) {
        m_globalSettingsErrorLabel->setText(u"结束时间必须晚于开始时间，且午餐与晚餐时间不能重叠。"_s);
        return;
    }

    m_globalSettingsErrorLabel->clear();
    if (!saveGlobalSettings()) {
        m_globalSettingsStatusLabel->setVisible(false);
        m_globalSettingsErrorLabel->setVisible(true);
        m_globalSettingsErrorLabel->setText(u"全局设置无法保存，请检查设置存储权限。"_s);
        return;
    }

    m_globalSettingsStatusLabel->setText(u"已保存 · 仅影响新记录"_s);
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

bool AttendanceMainWindow::migrateLegacyRecordsToCurrentSchedule()
{
    QSettings settings;
    return AttendanceSettings::migrateLegacyRecords(settings, currentSchedule());
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
                                              .excludedFromTarget = !record.needAverageCal,
                                          });
        }
    }

    const int targetMinutesPerDay = qRound(m_targetOvertimeHoursSpinBox->value() * 60.0);
    const MonthlyStatistics statistics =
        MonthlyStatisticsCalculator::calculate(records, mealSubsidyEnabled, overtimeOffsetsMissingWork);

    m_statsPeriodLabel->setText(u"%1年%2月"_s.arg(dates.first.year()).arg(dates.first.month()));
    m_statsWorkDaysValueLabel->setText(u"%1天"_s.arg(statistics.workDays));
    m_statsMissingWorkValueLabel->setText(AttendanceFormatter::formatMinutes(statistics.missingWorkMinutes));

    const bool hasOvertimeTarget = targetMinutesPerDay > 0;
    m_statsTargetLabel->setVisible(hasOvertimeTarget);
    m_statsTargetValueLabel->setVisible(hasOvertimeTarget);
    if (hasOvertimeTarget) {
        m_statsOvertimeLabel->setText(u"日均加班"_s);
        const int averageOvertimeMinutes =
            statistics.workDays > 0 ? qRound(statistics.overtimeMinutes / static_cast<double>(statistics.workDays)) : 0;
        m_statsOvertimeValueLabel->setText(AttendanceFormatter::formatMinutes(averageOvertimeMinutes));

        const int targetDifference = statistics.overtimeMinutes - targetMinutesPerDay * statistics.workDays;
        if (statistics.workDays == 0) {
            m_statsTargetValueLabel->setText(u"--"_s);
            setStyleProperty(*m_statsTargetValueLabel, "tone", u"neutral"_s);
        }
        else if (targetDifference < 0) {
            m_statsTargetValueLabel->setText(u"还差 %1"_s.arg(AttendanceFormatter::formatMinutes(-targetDifference)));
            setStyleProperty(*m_statsTargetValueLabel, "tone", u"warning"_s);
        }
        else if (targetDifference > 0) {
            m_statsTargetValueLabel->setText(u"超出 %1"_s.arg(AttendanceFormatter::formatMinutes(targetDifference)));
            setStyleProperty(*m_statsTargetValueLabel, "tone", u"positive"_s);
        }
        else {
            m_statsTargetValueLabel->setText(u"已达标"_s);
            setStyleProperty(*m_statsTargetValueLabel, "tone", u"positive"_s);
        }
    }
    else {
        m_statsOvertimeLabel->setText(u"总加班"_s);
        m_statsOvertimeValueLabel->setText(AttendanceFormatter::formatMinutes(statistics.overtimeMinutes));
    }

    m_statsMealLabel->setVisible(mealSubsidyEnabled);
    m_statsMealValueLabel->setVisible(mealSubsidyEnabled);
    m_statsMealValueLabel->setText(u"%1次"_s.arg(statistics.mealSubsidyCount));
}
