#include "Settings/TimeSettingDialog.h"
#include "Attendance/WorkTimeCalculator.h"
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
    m_globalDefaults(loadGlobalTimeDefaults())
{
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

void TimeSettingDialog::calculateWorkTime()
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

    auto fmtMin = [](int minutes) {
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
        resultText += QString("[加班时间] %1").arg(fmtMin(result.overtimeMinutes));
    }
    else if (result.overtimeMinutes < 0) {
        resultText += QString("[欠缺时间] %1").arg(fmtMin(-result.overtimeMinutes));
    }
    else {
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
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

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
    m_resultLabel->setFixedHeight(130);
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
    QString key = m_date.toString("yyyy-MM-dd");

    int dayOfWeek = m_date.dayOfWeek();
    bool defaultNeedAverage = (dayOfWeek != 6 && dayOfWeek != 7);
    m_needAverageCalCheckBox->setChecked(settings.value(key + "/needAverageCal", defaultNeedAverage).toBool());

    AttendanceRecord defaults;
    AttendanceRecord currentSchedule = m_globalDefaults;

    auto readScheduleTime = [&](const QString& name, const QTime& fallback) {
        QTime value =
            QTime::fromString(settings.value(key + "/" + name, fallback.toString("hh:mm")).toString(), "hh:mm");
        return value.isValid() ? value : fallback;
    };

    m_globalDefaults.workStartTime = readScheduleTime("workStart", m_globalDefaults.workStartTime);
    m_globalDefaults.workEndTime = readScheduleTime("workEnd", m_globalDefaults.workEndTime);
    m_globalDefaults.lunchBreakStart = readScheduleTime("lunchStart", m_globalDefaults.lunchBreakStart);
    m_globalDefaults.lunchBreakEnd = readScheduleTime("lunchEnd", m_globalDefaults.lunchBreakEnd);
    m_globalDefaults.dinnerBreakStart = readScheduleTime("dinnerStart", m_globalDefaults.dinnerBreakStart);
    m_globalDefaults.dinnerBreakEnd = readScheduleTime("dinnerEnd", m_globalDefaults.dinnerBreakEnd);
    m_globalDefaults.mealSubsidyTime = readScheduleTime("mealSubsidy", m_globalDefaults.mealSubsidyTime);
    if (!WorkTimeCalculator::hasValidSchedule(m_globalDefaults)) {
        m_globalDefaults = currentSchedule;
    }

    const bool hasStoredRecord = settings.contains(key + "/arrival");
    QTime arrivalTime =
        QTime::fromString(settings.value(key + "/arrival", defaults.arrivalTime.toString("hh:mm")).toString(),
                          "hh:mm");
    const QTime departureTime =
        QTime::fromString(settings.value(key + "/departure", defaults.departureTime.toString("hh:mm")).toString(),
                          "hh:mm");

    if (!hasStoredRecord && m_date == QDate::currentDate()) {
        const QTime now = QTime::currentTime();
        const QTime currentMinute(now.hour(), now.minute());
        if (currentMinute < departureTime) {
            arrivalTime = currentMinute;
        }
    }

    const QSignalBlocker arrivalBlocker(m_arrivalTimeEdit);
    const QSignalBlocker departureBlocker(m_departureTimeEdit);
    m_arrivalTimeEdit->setTime(arrivalTime);
    m_departureTimeEdit->setTime(departureTime);
    calculateWorkTime();
}

void TimeSettingDialog::saveRecord()
{
    QSettings settings;
    QString key = m_date.toString("yyyy-MM-dd");

    settings.setValue(key + "/needAverageCal", m_needAverageCalCheckBox->isChecked());
    settings.setValue(key + "/arrival", m_arrivalTimeEdit->time().toString("hh:mm"));
    settings.setValue(key + "/departure", m_departureTimeEdit->time().toString("hh:mm"));
    settings.setValue(key + "/workStart", m_globalDefaults.workStartTime.toString("hh:mm"));
    settings.setValue(key + "/workEnd", m_globalDefaults.workEndTime.toString("hh:mm"));
    settings.setValue(key + "/lunchStart", m_globalDefaults.lunchBreakStart.toString("hh:mm"));
    settings.setValue(key + "/lunchEnd", m_globalDefaults.lunchBreakEnd.toString("hh:mm"));
    settings.setValue(key + "/dinnerStart", m_globalDefaults.dinnerBreakStart.toString("hh:mm"));
    settings.setValue(key + "/dinnerEnd", m_globalDefaults.dinnerBreakEnd.toString("hh:mm"));
    settings.setValue(key + "/mealSubsidy", m_globalDefaults.mealSubsidyTime.toString("hh:mm"));
}
