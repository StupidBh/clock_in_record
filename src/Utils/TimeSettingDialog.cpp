#include "TimeSettingDialog.h"
#include "WorkTimeCalculator.h"
#include <QVBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QSettings>
#include <qDebug>

TimeSettingDialog::TimeSettingDialog(const QDate& date, QWidget* parent) :
    QDialog(parent),
    m_date(date)
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

    QSettings settings;
    AttendanceRecord defaults;
    record.workStartTime =
        QTime::fromString(settings.value("settings/workStart", defaults.workStartTime.toString("hh:mm")).toString(),
                          "hh:mm");
    record.workEndTime =
        QTime::fromString(settings.value("settings/workEnd", defaults.workEndTime.toString("hh:mm")).toString(),
                          "hh:mm");
    record.lunchBreakStart =
        QTime::fromString(settings.value("settings/lunchStart", defaults.lunchBreakStart.toString("hh:mm")).toString(),
                          "hh:mm");
    record.lunchBreakEnd =
        QTime::fromString(settings.value("settings/lunchEnd", defaults.lunchBreakEnd.toString("hh:mm")).toString(),
                          "hh:mm");
    record.dinnerBreakStart = QTime::fromString(
        settings.value("settings/dinnerStart", defaults.dinnerBreakStart.toString("hh:mm")).toString(),
        "hh:mm");
    record.dinnerBreakEnd =
        QTime::fromString(settings.value("settings/dinnerEnd", defaults.dinnerBreakEnd.toString("hh:mm")).toString(),
                          "hh:mm");
    return record;
}

void TimeSettingDialog::calculateWorkTime()
{
    AttendanceRecord record = getRecord();
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
        resultText.clear();
        resultText += QString("[今日无缺]");
    }

    m_resultLabel->setText(resultText);
}

void TimeSettingDialog::saveAndClose()
{
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

    QTime arrivalDefault = settings.contains(key + "/arrival") ? defaults.arrivalTime : QTime::currentTime();
    m_arrivalTimeEdit->setTime(
        QTime::fromString(settings.value(key + "/arrival", arrivalDefault.toString("hh:mm")).toString(), "hh:mm"));

    m_departureTimeEdit->setTime(
        QTime::fromString(settings.value(key + "/departure", defaults.departureTime.toString("hh:mm")).toString(),
                          "hh:mm"));
}

void TimeSettingDialog::saveRecord()
{
    QSettings settings;
    QString key = m_date.toString("yyyy-MM-dd");

    settings.setValue(key + "/needAverageCal", m_needAverageCalCheckBox->isChecked());
    settings.setValue(key + "/arrival", m_arrivalTimeEdit->time().toString("hh:mm"));
    settings.setValue(key + "/departure", m_departureTimeEdit->time().toString("hh:mm"));
}
