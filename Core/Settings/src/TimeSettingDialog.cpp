#include "Settings/TimeSettingDialog.h"

#include "Attendance/AttendanceFormatter.h"
#include "Attendance/WorkTimeCalculator.h"
#include "Settings/AttendanceSettings.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QStyle>
#include <QTimeEdit>
#include <QVBoxLayout>

using namespace Qt::StringLiterals;

TimeSettingDialog::TimeSettingDialog(const QDate& date, QWidget* parent) :
    QDialog(parent),
    m_date(date)
{
    QSettings settings;
    const AttendanceSettings::GlobalSettings globalSettings = AttendanceSettings::loadGlobalSettings(settings);
    m_globalDefaults = globalSettings.schedule;
    m_overtimeOffsetsMissingWork = globalSettings.overtimeOffsetsMissingWork;

    setWindowTitle(u"设置打卡时间 - %1"_s.arg(date.toString(u"yyyy-MM-dd"_s)));
    setModal(true);
    setMinimumSize(380, 360);
    resize(440, 420);

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

    if (!WorkTimeCalculator::hasValidAttendanceRange(currentRecord)) {
        m_resultLabel->setText(u"离开时间必须晚于到达时间。"_s);
        return;
    }
    if (!WorkTimeCalculator::hasValidSchedule(currentRecord)) {
        m_resultLabel->setText(u"该日期使用的作息配置无效。"_s);
        return;
    }

    WorkTimeResult result = WorkTimeCalculator::calculateWorkTimeResult(currentRecord);
    result = WorkTimeCalculator::applyOvertimeOffset(result, m_overtimeOffsetsMissingWork);
    m_resultLabel->setText(AttendanceFormatter::formatDailyResult(result));
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

    saveRecord();
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
    AttendanceSettings::removeRecord(settings, m_date);
    accept();
}

void TimeSettingDialog::setupUi()
{
    auto* const mainLayout = new QVBoxLayout(this);

    auto* const basicTimeGroup = new QGroupBox(u"基本时间"_s);
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

    m_needAverageCalCheckBox = new QCheckBox(u"计入平均加班日"_s);
    m_needAverageCalCheckBox->setChecked(true);
    mainLayout->addWidget(m_needAverageCalCheckBox);

    auto* const resultGroup = new QGroupBox(u"计算结果"_s);
    auto* const resultLayout = new QVBoxLayout(resultGroup);
    m_resultLabel = new QLabel();
    m_resultLabel->setObjectName(u"calculationResultLabel"_s);
    m_resultLabel->setWordWrap(true);
    m_resultLabel->setMinimumHeight(120);
    m_resultLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
    resultLayout->addWidget(m_resultLabel);
    mainLayout->addWidget(resultGroup, 1);

    auto* const buttonBox = new QDialogButtonBox();
    m_deleteButton = new QPushButton(u"删除记录"_s);
    m_deleteButton->setObjectName(u"deleteRecordButton"_s);
    m_deleteButton->setIcon(style()->standardIcon(QStyle::SP_TrashIcon));
    buttonBox->addButton(m_deleteButton, QDialogButtonBox::DestructiveRole);
    auto* const cancelButton = buttonBox->addButton(u"取消"_s, QDialogButtonBox::RejectRole);
    auto* const saveButton = buttonBox->addButton(u"保存"_s, QDialogButtonBox::AcceptRole);
    saveButton->setObjectName(u"primaryButton"_s);
    saveButton->setDefault(true);

    connect(m_deleteButton, &QPushButton::clicked, this, &TimeSettingDialog::deleteAndClose);
    connect(saveButton, &QPushButton::clicked, this, &TimeSettingDialog::saveAndClose);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    connect(m_arrivalTimeEdit, &QTimeEdit::timeChanged, this, &TimeSettingDialog::calculateWorkTime);
    connect(m_departureTimeEdit, &QTimeEdit::timeChanged, this, &TimeSettingDialog::calculateWorkTime);
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
    m_needAverageCalCheckBox->setChecked(loadedRecord.needAverageCal);
    m_arrivalTimeEdit->setTime(loadedRecord.arrivalTime);
    m_departureTimeEdit->setTime(loadedRecord.departureTime);
    calculateWorkTime();
}

void TimeSettingDialog::saveRecord() const
{
    QSettings settings;
    AttendanceSettings::saveRecord(settings, m_date, record());
}
