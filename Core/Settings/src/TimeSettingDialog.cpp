#include "Settings/TimeSettingDialog.h"
#include "Attendance/AttendanceFormatter.h"
#include "Attendance/WorkTimeCalculator.h"
#include "Settings/AttendanceSettings.h"

#include <QCheckBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSignalBlocker>
#include <QStringList>
#include <QStringView>
#include <QStyle>
#include <QTimeEdit>
#include <QVariant>

#include <array>

using namespace Qt::StringLiterals;

namespace {
    QLabel* addResultMetric(QGridLayout& layout, const QStringView label, const int row, const int pair)
    {
        const int firstColumn = pair * 2;
        auto* const metricLabel = new QLabel(label.toString());
        metricLabel->setProperty("resultRole", u"metric"_s);
        auto* const valueLabel = new QLabel();
        valueLabel->setProperty("resultRole", u"value"_s);
        layout.addWidget(metricLabel, row, firstColumn);
        layout.addWidget(valueLabel, row, firstColumn + 1);
        return valueLabel;
    }

    void setTone(QLabel& label, const QStringView tone)
    {
        const QVariant value = tone.toString();
        if (label.property("tone") == value) {
            return;
        }

        label.setProperty("tone", value);
        label.style()->unpolish(&label);
        label.style()->polish(&label);
        label.update();
    }
} // namespace

TimeSettingDialog::TimeSettingDialog(const QDate& date, QWidget* parent) :
    QDialog(parent),
    m_date(date)
{
    QSettings settings;
    const AttendanceSettings::GlobalSettings globalSettings = AttendanceSettings::loadGlobalSettings(settings);
    m_globalDefaults = globalSettings.schedule;
    m_overtimeOffsetsMissingWork = globalSettings.overtimeOffsetsMissingWork;

    setWindowTitle(u"考勤记录 - %1"_s.arg(date.toString(u"yyyy-MM-dd"_s)));
    setModal(true);
    setMinimumSize(420, 400);
    resize(480, 450);

    setupUi();
    loadRecord();
}

AttendanceRecord TimeSettingDialog::record() const
{
    AttendanceRecord record;
    record.needAverageCal = m_needAverageCalCheckBox->isChecked();
    record.arrivalTime = m_arrivalTimeEdit->time();
    record.departureTime = m_departureTimeEdit->time();

    record.workStartTime = m_globalDefaults.workStartTime;
    record.workEndTime = m_globalDefaults.workEndTime;
    record.lunchBreakStart = m_globalDefaults.lunchBreakStart;
    record.lunchBreakEnd = m_globalDefaults.lunchBreakEnd;
    record.dinnerBreakStart = m_globalDefaults.dinnerBreakStart;
    record.dinnerBreakEnd = m_globalDefaults.dinnerBreakEnd;
    record.mealSubsidyTime = m_globalDefaults.mealSubsidyTime;
    return record;
}

void TimeSettingDialog::calculateWorkTime() const
{
    const AttendanceRecord currentRecord = record();
    const std::array metricLabels {
        m_actualWorkValueLabel,
        m_standardWorkValueLabel,
        m_breakValueLabel,
        m_overtimeValueLabel,
    };

    const auto clearMetrics = [&metricLabels]() {
        for (QLabel* const label : metricLabels) {
            label->setText(u"--"_s);
        }
    };

    if (!WorkTimeCalculator::hasValidAttendanceRange(currentRecord)) {
        m_resultLabel->setText(u"时间范围无效"_s);
        m_resultDetailLabel->setText(u"离开时间必须晚于到达时间"_s);
        m_resultDetailLabel->setVisible(true);
        setTone(*m_resultLabel, u"warning");
        clearMetrics();
        return;
    }
    if (!WorkTimeCalculator::hasValidSchedule(currentRecord)) {
        m_resultLabel->setText(u"作息配置无效"_s);
        m_resultDetailLabel->setText(u"请检查全局工作时间设置"_s);
        m_resultDetailLabel->setVisible(true);
        setTone(*m_resultLabel, u"warning");
        clearMetrics();
        return;
    }

    WorkTimeResult result = WorkTimeCalculator::calculateWorkTimeResult(currentRecord);
    result = WorkTimeCalculator::applyOvertimeOffset(result, m_overtimeOffsetsMissingWork);
    m_actualWorkValueLabel->setText(AttendanceFormatter::formatMinutes(result.actualWorkMinutes));
    m_standardWorkValueLabel->setText(AttendanceFormatter::formatMinutes(result.standardWorkMinutes));
    m_breakValueLabel->setText(AttendanceFormatter::formatMinutes(result.totalBreakMinutes));
    m_overtimeValueLabel->setText(AttendanceFormatter::formatMinutes(result.overtimeMinutes));

    if (result.missingWorkMinutes > 0) {
        m_resultLabel->setText(u"缺少标准工时 %1"_s.arg(AttendanceFormatter::formatMinutes(result.missingWorkMinutes)));
        setTone(*m_resultLabel, u"warning");
    }
    else if (result.overtimeMinutes > 0) {
        m_resultLabel->setText(u"加班 %1"_s.arg(AttendanceFormatter::formatMinutes(result.overtimeMinutes)));
        setTone(*m_resultLabel, u"positive");
    }
    else {
        m_resultLabel->setText(u"今日工时达标"_s);
        setTone(*m_resultLabel, u"positive");
    }

    QStringList details;
    if (result.lateMinutes > 0) {
        details.append(u"迟到 %1"_s.arg(AttendanceFormatter::formatMinutes(result.lateMinutes)));
    }
    if (result.earlyLeaveMinutes > 0) {
        details.append(u"早退 %1"_s.arg(AttendanceFormatter::formatMinutes(result.earlyLeaveMinutes)));
    }
    if (!currentRecord.needAverageCal) {
        details.append(u"不计入月度加班目标"_s);
    }
    m_resultDetailLabel->setText(details.join(u" · "_s));
    m_resultDetailLabel->setVisible(!details.isEmpty());
}

void TimeSettingDialog::saveAndClose()
{
    const AttendanceRecord currentRecord = record();
    if (!WorkTimeCalculator::hasValidAttendanceRange(currentRecord)) {
        QMessageBox::warning(this, u"时间无效"_s, u"离开时间必须晚于到达时间。"_s);
        return;
    }
    if (!WorkTimeCalculator::hasValidSchedule(currentRecord)) {
        QMessageBox::warning(this, u"作息无效"_s, u"该日期使用的作息配置无效，请检查全局设置。"_s);
        return;
    }

    if (!saveRecord()) {
        QMessageBox::warning(this, u"保存失败"_s, u"考勤记录无法保存，请检查设置存储权限。"_s);
        return;
    }

    accept();
}

void TimeSettingDialog::deleteAndClose()
{
    const auto response = QMessageBox::question(this,
                                                u"确认删除"_s,
                                                u"确定要删除 %1 的考勤记录吗？"_s.arg(m_date.toString(u"yyyy-MM-dd"_s)),
                                                QMessageBox::Yes | QMessageBox::No,
                                                QMessageBox::No);
    if (response != QMessageBox::Yes) {
        return;
    }

    QSettings settings;
    if (!AttendanceSettings::removeRecord(settings, m_date)) {
        QMessageBox::warning(this, u"删除失败"_s, u"考勤记录无法删除，请检查设置存储权限。"_s);
        return;
    }

    accept();
}

void TimeSettingDialog::setupUi()
{
    auto* const mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(18, 16, 18, 16);
    mainLayout->setSpacing(12);

    auto* const dateLabel =
        new QLabel(QLocale(QLocale::Chinese, QLocale::China).toString(m_date, u"yyyy年M月d日 dddd"_s));
    dateLabel->setObjectName(u"dialogDateLabel"_s);
    mainLayout->addWidget(dateLabel);

    auto* const basicTimeGroup = new QGroupBox(u"打卡时间"_s);
    auto* const basicTimeLayout = new QGridLayout(basicTimeGroup);
    basicTimeLayout->setColumnStretch(1, 1);

    basicTimeLayout->addWidget(new QLabel(u"到达时间："_s), 0, 0);
    m_arrivalTimeEdit = new QTimeEdit();
    m_arrivalTimeEdit->setDisplayFormat(u"hh:mm"_s);
    basicTimeLayout->addWidget(m_arrivalTimeEdit, 0, 1);

    basicTimeLayout->addWidget(new QLabel(u"离开时间："_s), 1, 0);
    m_departureTimeEdit = new QTimeEdit();
    m_departureTimeEdit->setDisplayFormat(u"hh:mm"_s);
    basicTimeLayout->addWidget(m_departureTimeEdit, 1, 1);

    mainLayout->addWidget(basicTimeGroup);

    m_needAverageCalCheckBox = new QCheckBox(u"计入加班目标统计"_s);
    m_needAverageCalCheckBox->setChecked(true);
    m_needAverageCalCheckBox->setToolTip(u"仅影响工作日数量和加班目标比较，总加班时长仍会统计"_s);
    mainLayout->addWidget(m_needAverageCalCheckBox);

    auto* const resultTitleLabel = new QLabel(u"计算结果"_s);
    resultTitleLabel->setObjectName(u"sectionTitleLabel"_s);
    mainLayout->addWidget(resultTitleLabel);

    auto* const resultPanel = new QWidget();
    resultPanel->setObjectName(u"calculationResultPanel"_s);
    auto* const resultLayout = new QVBoxLayout(resultPanel);
    resultLayout->setContentsMargins(14, 12, 14, 12);
    resultLayout->setSpacing(8);
    m_resultLabel = new QLabel();
    m_resultLabel->setObjectName(u"calculationResultLabel"_s);
    m_resultLabel->setWordWrap(true);
    resultLayout->addWidget(m_resultLabel);
    m_resultDetailLabel = new QLabel();
    m_resultDetailLabel->setObjectName(u"calculationResultDetailLabel"_s);
    m_resultDetailLabel->setWordWrap(true);
    resultLayout->addWidget(m_resultDetailLabel);

    auto* const resultMetricsLayout = new QGridLayout();
    resultMetricsLayout->setContentsMargins(0, 6, 0, 0);
    resultMetricsLayout->setHorizontalSpacing(10);
    resultMetricsLayout->setVerticalSpacing(7);
    resultMetricsLayout->setColumnStretch(1, 1);
    resultMetricsLayout->setColumnStretch(3, 1);
    m_actualWorkValueLabel = addResultMetric(*resultMetricsLayout, u"实际工时"_s, 0, 0);
    m_actualWorkValueLabel->setObjectName(u"actualWorkValueLabel"_s);
    m_standardWorkValueLabel = addResultMetric(*resultMetricsLayout, u"标准工时"_s, 0, 1);
    m_standardWorkValueLabel->setObjectName(u"standardWorkValueLabel"_s);
    m_breakValueLabel = addResultMetric(*resultMetricsLayout, u"休息时间"_s, 1, 0);
    m_breakValueLabel->setObjectName(u"breakValueLabel"_s);
    m_overtimeValueLabel = addResultMetric(*resultMetricsLayout, u"加班时间"_s, 1, 1);
    m_overtimeValueLabel->setObjectName(u"overtimeValueLabel"_s);
    resultLayout->addLayout(resultMetricsLayout);
    mainLayout->addWidget(resultPanel, 1);

    auto* const actionLayout = new QHBoxLayout();
    actionLayout->setContentsMargins(0, 0, 0, 0);
    m_deleteButton = new QPushButton(u"删除记录"_s);
    m_deleteButton->setObjectName(u"deleteRecordButton"_s);
    actionLayout->addWidget(m_deleteButton);
    actionLayout->addStretch();
    auto* const cancelButton = new QPushButton(u"取消"_s);
    cancelButton->setObjectName(u"cancelButton"_s);
    actionLayout->addWidget(cancelButton);
    auto* const saveButton = new QPushButton(u"保存"_s);
    saveButton->setObjectName(u"primaryButton"_s);
    saveButton->setDefault(true);
    actionLayout->addWidget(saveButton);

    connect(m_deleteButton, &QPushButton::clicked, this, &TimeSettingDialog::deleteAndClose);
    connect(saveButton, &QPushButton::clicked, this, &TimeSettingDialog::saveAndClose);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    mainLayout->addLayout(actionLayout);

    connect(m_arrivalTimeEdit, &QTimeEdit::timeChanged, this, &TimeSettingDialog::calculateWorkTime);
    connect(m_departureTimeEdit, &QTimeEdit::timeChanged, this, &TimeSettingDialog::calculateWorkTime);
    connect(m_needAverageCalCheckBox, &QCheckBox::toggled, this, &TimeSettingDialog::calculateWorkTime);
}

void TimeSettingDialog::loadRecord()
{
    QSettings settings;
    const auto storedRecord = AttendanceSettings::loadRecord(settings, m_date, m_globalDefaults);
    m_deleteButton->setVisible(storedRecord.has_value());
    AttendanceRecord loadedRecord = storedRecord.value_or(AttendanceSettings::createRecord(m_date, m_globalDefaults));
    m_globalDefaults = loadedRecord;

    if (!storedRecord && m_date == QDate::currentDate()) {
        const QTime now = QTime::currentTime();
        const QTime currentMinute(now.hour(), now.minute());
        if (currentMinute < loadedRecord.departureTime) {
            loadedRecord.arrivalTime = currentMinute;
        }
    }

    const QSignalBlocker arrivalBlocker(m_arrivalTimeEdit);
    const QSignalBlocker departureBlocker(m_departureTimeEdit);
    const QSignalBlocker averageBlocker(m_needAverageCalCheckBox);
    m_needAverageCalCheckBox->setChecked(loadedRecord.needAverageCal);
    m_arrivalTimeEdit->setTime(loadedRecord.arrivalTime);
    m_departureTimeEdit->setTime(loadedRecord.departureTime);
    calculateWorkTime();
}

bool TimeSettingDialog::saveRecord() const
{
    QSettings settings;
    return AttendanceSettings::saveRecord(settings, m_date, record());
}
