#include "Settings/TimeSettingDialog.h"
#include "Attendance/WorkTimeCalculator.h"
#include "Settings/AttendanceSettings.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSignalBlocker>

TimeSettingDialog::TimeSettingDialog(const QDate& date, QWidget* parent) :
    QDialog(parent),
    m_date(date),
    m_globalDefaults(),
    m_overtimeOffsetsMissingWork(false)
{
    QSettings settings;
    const AttendanceSettings::GlobalSettings globalSettings = AttendanceSettings::loadGlobalSettings(settings);
    m_globalDefaults = globalSettings.schedule;
    m_overtimeOffsetsMissingWork = globalSettings.overtimeOffsetsMissingWork;

    setWindowTitle(QString("设置打卡时间 - %1").arg(date.toString("yyyy-MM-dd")));
    setModal(true);
    resize(400, 320);

    setupUI();
    loadRecord();
}

AttendanceRecord TimeSettingDialog::getRecord() const
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
    AttendanceRecord record = getRecord();

    if (!WorkTimeCalculator::hasValidAttendanceRange(record)) {
        m_resultLabel->setText(QString("离开时间必须晚于到达时间。"));
        return;
    }
    if (!WorkTimeCalculator::hasValidSchedule(record)) {
        m_resultLabel->setText(QString("该日期使用的作息配置无效。"));
        return;
    }

    WorkTimeResult result = WorkTimeCalculator::calculateWorkTimeResult(record);
    result = WorkTimeCalculator::applyOvertimeOffset(result, m_overtimeOffsetsMissingWork);

    auto fmtMin = [](const int minutes) {
        return QString("%1小时%2分钟").arg(minutes / 60).arg(minutes % 60);
    };

    QString resultText;

    if (result.lateMinutes > 0) {
        resultText += QString("[迟到] %1\n").arg(fmtMin(result.lateMinutes));
    }

    if (result.earlyLeaveMinutes > 0) {
        resultText += QString("[早退] %1\n").arg(fmtMin(result.earlyLeaveMinutes));
    }

    resultText += QString("[标准工时] %1\n\n").arg(fmtMin(result.standardWorkMinutes));
    resultText += QString("[实际工时] %1\n").arg(fmtMin(result.actualWorkMinutes));

    if (result.totalBreakMinutes > 0) {
        resultText += QString("[休息时间] %1\n").arg(fmtMin(result.totalBreakMinutes));
    }
    if (result.overtimeMinutes > 0) {
        resultText += QString("[加班时间] %1\n").arg(fmtMin(result.overtimeMinutes));
    }
    if (result.missingWorkMinutes > 0) {
        resultText += QString("[缺少标准工时] %1").arg(fmtMin(result.missingWorkMinutes));
    }
    if (result.overtimeMinutes == 0 && result.missingWorkMinutes == 0) {
        resultText += QString("[今日无缺]");
    }

    m_resultLabel->setText(resultText);
}

void TimeSettingDialog::saveAndClose()
{
    AttendanceRecord record = getRecord();
    if (!WorkTimeCalculator::hasValidAttendanceRange(record)) {
        QMessageBox::warning(this, QString("时间无效"), QString("离开时间必须晚于到达时间。"));
        return;
    }
    if (!WorkTimeCalculator::hasValidSchedule(record)) {
        QMessageBox::warning(this, QString("作息无效"), QString("该日期使用的作息配置无效，请检查全局设置。"));
        return;
    }

    saveRecord();
    accept();
}

void TimeSettingDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this); // NOLINT(*-use-auto)

    // 基本时间设置组
    QGroupBox* basicTimeGroup = new QGroupBox(QString("基本时间"));
    QGridLayout* basicTimeLayout = new QGridLayout(basicTimeGroup);

    basicTimeLayout->addWidget(new QLabel(QString("到达公司时间:")), 0, 0);
    m_arrivalTimeEdit = new QTimeEdit();
    m_arrivalTimeEdit->setDisplayFormat("hh:mm");
    basicTimeLayout->addWidget(m_arrivalTimeEdit, 0, 1);

    basicTimeLayout->addWidget(new QLabel(QString("离开公司时间:")), 1, 0);
    m_departureTimeEdit = new QTimeEdit();
    m_departureTimeEdit->setDisplayFormat("hh:mm");
    basicTimeLayout->addWidget(m_departureTimeEdit, 1, 1);

    mainLayout->addWidget(basicTimeGroup);

    // 计入平均加班日
    m_needAverageCalCheckBox = new QCheckBox(QString("计入平均加班日"));
    m_needAverageCalCheckBox->setChecked(true);
    mainLayout->addWidget(m_needAverageCalCheckBox);

    // 计算结果
    QGroupBox* resultGroup = new QGroupBox(QString("计算结果"));
    QVBoxLayout* resultLayout = new QVBoxLayout(resultGroup);
    m_resultLabel = new QLabel(QString(""));
    m_resultLabel->setWordWrap(true);
    m_resultLabel->setFixedHeight(150);
    m_resultLabel->setStyleSheet("padding: 10px; background-color: #f0f0f0; border-radius: 5px;");
    resultLayout->addWidget(m_resultLabel);
    mainLayout->addWidget(resultGroup);

    // 按钮布局
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* saveBtn = new QPushButton(QString("保存"));
    QPushButton* cancelBtn = new QPushButton(QString("取消"));

    connect(saveBtn, &QPushButton::clicked, this, &TimeSettingDialog::saveAndClose);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    buttonLayout->addStretch();
    buttonLayout->addWidget(saveBtn);
    buttonLayout->addWidget(cancelBtn);
    mainLayout->addLayout(buttonLayout);

    connect(m_arrivalTimeEdit, &QTimeEdit::timeChanged, this, &TimeSettingDialog::calculateWorkTime);
    connect(m_departureTimeEdit, &QTimeEdit::timeChanged, this, &TimeSettingDialog::calculateWorkTime);
}

void TimeSettingDialog::loadRecord()
{
    QSettings settings;
    const auto storedRecord = AttendanceSettings::loadRecord(settings, m_date, m_globalDefaults);
    AttendanceRecord record = storedRecord.value_or(AttendanceSettings::createRecord(m_date, m_globalDefaults));
    m_globalDefaults = record;

    if (!storedRecord && m_date == QDate::currentDate()) {
        const QTime now = QTime::currentTime();
        const QTime currentMinute(now.hour(), now.minute());
        if (currentMinute < record.departureTime) {
            record.arrivalTime = currentMinute;
        }
    }

    const QSignalBlocker arrivalBlocker(m_arrivalTimeEdit);
    const QSignalBlocker departureBlocker(m_departureTimeEdit);
    m_needAverageCalCheckBox->setChecked(record.needAverageCal);
    m_arrivalTimeEdit->setTime(record.arrivalTime);
    m_departureTimeEdit->setTime(record.departureTime);
    calculateWorkTime();
}

void TimeSettingDialog::saveRecord() const
{
    QSettings settings;
    AttendanceSettings::saveRecord(settings, m_date, getRecord());
}
