#include <QtWidgets>
#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QCalendarWidget>
#include <QDialog>
#include <QLabel>
#include <QTimeEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QSpinBox>
#include <QGroupBox>
#include <QMessageBox>
#include <QSettings>
#include <QDate>
#include <QTime>
#include <QMap>
#include <QSplitter>
#include <QFrame>
#include <QTextCodec>
#include <QSizePolicy>
#pragma execution_character_set("utf-8")
#if defined(_MSC_VER) && (_MSC_VER >= 1600)
# pragma execution_character_set("utf-8")
#endif

// 自定义可折叠组框
class CollapsibleGroupBox : public QWidget {
    Q_OBJECT // 需要Q_OBJECT宏以支持信号槽

public:
    CollapsibleGroupBox(const QString &title, QWidget *parent = nullptr)
        : QWidget(parent), m_collapsed(true), m_title(title) {

        m_toggleButton = new QPushButton();
        m_toggleButton->setCheckable(true);
        m_toggleButton->setChecked(false);
        m_toggleButton->setStyleSheet(
            "QPushButton { text-align: left; border: none; font-weight: bold; padding: 5px; }"
            "QPushButton:checked { background-color: #e0e0e0; }"
            );

        m_contentWidget = new QWidget();
        m_contentWidget->setWindowFlags(Qt::Popup);
        m_contentWidget->setVisible(false);

        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->addWidget(m_toggleButton);
        // layout->addWidget(m_contentWidget);
        layout->setContentsMargins(0, 0, 0, 0);

        connect(m_toggleButton, &QPushButton::toggled, this, &CollapsibleGroupBox::toggle);

        updateButtonText();
    }

    void setContentLayout(QLayout *layout) {
        m_contentWidget->setLayout(layout);
        auto pw = this->parentWidget();
        if(pw){
            pw->resize(pw->size().width(),pw->sizeHint().height());
        }
    }

    QWidget* contentWidget() const {
        return m_contentWidget;
    }

private slots:
    void toggle(bool checked) {
        m_collapsed = !checked;
        m_contentWidget->setVisible(checked);
        QPoint globalPos = m_toggleButton->mapToGlobal(QPoint(0, 0));
        int x = globalPos.x();
        int y = globalPos.y() + m_toggleButton->height(); // 显示在按钮下方
        m_contentWidget->move(x, y);
        updateButtonText();
    }

private:
    void updateButtonText() {
        QString arrow = m_collapsed ? ">" : "v";
        m_toggleButton->setText(arrow + " " + m_title);
    }

    QPushButton *m_toggleButton;
    QWidget *m_contentWidget;
    bool m_collapsed;
    QString m_title;
};

// 打卡记录结构体
struct AttendanceRecord {
    QTime arrivalTime;      // 到公司时间
    QTime departureTime;    // 出公司时间
    QTime workStartTime;    // 标准上班时间
    QTime workEndTime;      // 标准下班时间
    QTime lunchBreakStart;  // 午休开始时间
    QTime lunchBreakEnd;    // 午休结束时间
    QTime dinnerBreakStart; // 晚饭开始时间
    QTime dinnerBreakEnd;   // 晚饭结束时间

    AttendanceRecord() {
        arrivalTime = QTime(9, 0);
        departureTime = QTime(18, 0);
        workStartTime = QTime(9, 0);
        workEndTime = QTime(18, 0);
        lunchBreakStart = QTime(12, 30);
        lunchBreakEnd = QTime(13, 30);
        dinnerBreakStart = QTime(18, 0);
        dinnerBreakEnd = QTime(18, 30);
    }
};

// 工作时间计算结果
struct WorkTimeResult {
    int actualWorkMinutes = 0;      // 实际工作时间（分钟）
    int standardWorkMinutes = 0;    // 标准工作时间（分钟）
    int lateMinutes = 0;           // 迟到时间（分钟）
    int lateSize = 0;
    int earlyLeaveMinutes = 0;     // 早退时间（分钟）
    int earlyLeaveSize = 0;
    int overtimeMinutes = 0;       // 加班时间（分钟）
    int totalBreakMinutes = 0;     // 总休息时间（分钟）
};

// 工作时间计算工具类
class WorkTimeCalculator {
public:
    static WorkTimeResult calculateWorkTimeResult(const AttendanceRecord& record) {
        WorkTimeResult result;

        // 计算迟到时间
        if (record.arrivalTime > record.workStartTime) {
            result.lateMinutes = record.workStartTime.secsTo(record.arrivalTime) / 60;
        }

        // 计算早退时间
        if (record.departureTime < record.workEndTime) {
            result.earlyLeaveMinutes = record.departureTime.secsTo(record.workEndTime) / 60;
        }

        // 计算在公司总时间
        int totalMinutesAtWork = record.arrivalTime.secsTo(record.departureTime) / 60;

        // 计算实际休息时间
        result.totalBreakMinutes = 0;

        // 午休时间
        if (isTimeRangeOverlap(record.arrivalTime, record.departureTime,
                               record.lunchBreakStart, record.lunchBreakEnd)) {
            QTime lunchStart = maxTime(record.arrivalTime, record.lunchBreakStart);
            QTime lunchEnd = minTime(record.departureTime, record.lunchBreakEnd);
            if (lunchStart < lunchEnd) {
                result.totalBreakMinutes += lunchStart.secsTo(lunchEnd) / 60;
            }
        }

        // 晚饭时间
        if (isTimeRangeOverlap(record.arrivalTime, record.departureTime,
                               record.dinnerBreakStart, record.dinnerBreakEnd)) {
            QTime dinnerStart = maxTime(record.arrivalTime, record.dinnerBreakStart);
            QTime dinnerEnd = minTime(record.departureTime, record.dinnerBreakEnd);
            if (dinnerStart < dinnerEnd) {
                result.totalBreakMinutes += dinnerStart.secsTo(dinnerEnd) / 60;
            }
        }

        // 实际工作时间 = 在公司时间 - 休息时间
        result.actualWorkMinutes = totalMinutesAtWork - result.totalBreakMinutes;

        // 标准工作时间
        int standardTotalMinutes = record.workStartTime.secsTo(record.workEndTime) / 60;
        int standardBreakMinutes = 0;

        // 标准午休时间
        if (record.lunchBreakStart < record.lunchBreakEnd) {
            standardBreakMinutes += record.lunchBreakStart.secsTo(record.lunchBreakEnd) / 60;
        }

        // 标准晚饭时间（如果在工作时间内）
        if (record.dinnerBreakStart >= record.workStartTime &&
            record.dinnerBreakStart < record.workEndTime) {
            QTime dinnerEnd = minTime(record.dinnerBreakEnd, record.workEndTime);
            if (record.dinnerBreakStart < dinnerEnd) {
                standardBreakMinutes += record.dinnerBreakStart.secsTo(dinnerEnd) / 60;
            }
        }

        result.standardWorkMinutes = standardTotalMinutes - standardBreakMinutes;

        // 加班时间
        result.overtimeMinutes = result.actualWorkMinutes - result.standardWorkMinutes;

        return result;
    }

private:
    // 辅助函数
    static bool isTimeRangeOverlap(const QTime& start1, const QTime& end1,
                                   const QTime& start2, const QTime& end2) {
        return start1 < end2 && start2 < end1;
    }

    static QTime maxTime(const QTime& time1, const QTime& time2) {
        return time1 > time2 ? time1 : time2;
    }

    static QTime minTime(const QTime& time1, const QTime& time2) {
        return time1 < time2 ? time1 : time2;
    }
};

// 时间设置对话框
class TimeSettingDialog : public QDialog {
    Q_OBJECT

public:
    TimeSettingDialog(const QDate& date, QWidget* parent = nullptr)
        : QDialog(parent), m_date(date) {
        setWindowTitle(QString("设置打卡时间 - %1").arg(date.toString("yyyy-MM-dd")));
        setModal(true);
        resize(450, 400);

        setupUI();
        loadRecord();
    }

    AttendanceRecord getRecord() const {
        AttendanceRecord record;
        record.arrivalTime = m_arrivalTimeEdit->time();
        record.departureTime = m_departureTimeEdit->time();
        record.workStartTime = m_workStartTimeEdit->time();
        record.workEndTime = m_workEndTimeEdit->time();
        record.lunchBreakStart = m_lunchBreakStartEdit->time();
        record.lunchBreakEnd = m_lunchBreakEndEdit->time();
        record.dinnerBreakStart = m_dinnerBreakStartEdit->time();
        record.dinnerBreakEnd = m_dinnerBreakEndEdit->time();
        return record;
    }

private slots:
    void calculateWorkTime() {
        AttendanceRecord record = getRecord();
        WorkTimeResult result = WorkTimeCalculator::calculateWorkTimeResult(record);

        QString resultText;

        // 显示迟到和早退
        if (result.lateMinutes > 0) {
            resultText += QString("[迟到] %1小时%2分钟\n")
                              .arg(result.lateMinutes / 60)
                              .arg(result.lateMinutes % 60);
        }

        if (result.earlyLeaveMinutes > 0) {
            resultText += QString("[早退] %1小时%2分钟\n")
                              .arg(result.earlyLeaveMinutes / 60)
                              .arg(result.earlyLeaveMinutes % 60);
        }

        // 显示工作时间
        resultText += QString("[实际工作] %1小时%2分钟\n")
                          .arg(result.actualWorkMinutes / 60)
                          .arg(result.actualWorkMinutes % 60);

        resultText += QString("[标准工作] %1小时%2分钟\n")
                          .arg(result.standardWorkMinutes / 60)
                          .arg(result.standardWorkMinutes % 60);

        resultText += QString("[总休息] %1小时%2分钟\n")
                          .arg(result.totalBreakMinutes / 60)
                          .arg(result.totalBreakMinutes % 60);

        // 显示加班或不足
        if (result.overtimeMinutes > 0) {
            resultText += QString("[加班时间] %1小时%2分钟")
                              .arg(result.overtimeMinutes / 60)
                              .arg(result.overtimeMinutes % 60);
        } else if (result.overtimeMinutes < 0) {
            resultText += QString("[工作不足] %1小时%2分钟")
                              .arg((-result.overtimeMinutes) / 60)
                              .arg((-result.overtimeMinutes) % 60);
        } else {
            resultText += QString("[正常工作时间]");
        }

        m_resultLabel->setText(resultText);
    }

    void saveAndClose() {
        saveRecord();
        accept();
    }

private:
    void setupUI() {
        QVBoxLayout* mainLayout = new QVBoxLayout(this);

        // 基本时间设置区域
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

        // 可折叠的详细设置
        CollapsibleGroupBox* detailsGroup = new CollapsibleGroupBox(QString("详细设置"), this);

        QVBoxLayout* detailsLayout = new QVBoxLayout();

        // 标准工作时间
        QGroupBox* standardGroup = new QGroupBox(QString("标准工作时间"));
        QGridLayout* standardLayout = new QGridLayout(standardGroup);

        standardLayout->addWidget(new QLabel(QString("标准上班时间:")), 0, 0);
        m_workStartTimeEdit = new QTimeEdit();
        m_workStartTimeEdit->setDisplayFormat("hh:mm");
        standardLayout->addWidget(m_workStartTimeEdit, 0, 1);

        standardLayout->addWidget(new QLabel(QString("标准下班时间:")), 1, 0);
        m_workEndTimeEdit = new QTimeEdit();
        m_workEndTimeEdit->setDisplayFormat("hh:mm");
        standardLayout->addWidget(m_workEndTimeEdit, 1, 1);



        // 休息时间设置
        QGroupBox* breakGroup = new QGroupBox(QString("休息时间设置"));
        QGridLayout* breakLayout = new QGridLayout(breakGroup);

        breakLayout->addWidget(new QLabel(QString("午休开始时间:")), 0, 0);
        m_lunchBreakStartEdit = new QTimeEdit();
        m_lunchBreakStartEdit->setDisplayFormat("hh:mm");
        breakLayout->addWidget(m_lunchBreakStartEdit, 0, 1);

        breakLayout->addWidget(new QLabel(QString("午休结束时间:")), 1, 0);
        m_lunchBreakEndEdit = new QTimeEdit();
        m_lunchBreakEndEdit->setDisplayFormat("hh:mm");
        breakLayout->addWidget(m_lunchBreakEndEdit, 1, 1);

        breakLayout->addWidget(new QLabel(QString("晚饭开始时间:")), 2, 0);
        m_dinnerBreakStartEdit = new QTimeEdit();
        m_dinnerBreakStartEdit->setDisplayFormat("hh:mm");
        breakLayout->addWidget(m_dinnerBreakStartEdit, 2, 1);

        breakLayout->addWidget(new QLabel(QString("晚饭结束时间:")), 3, 0);
        m_dinnerBreakEndEdit = new QTimeEdit();
        m_dinnerBreakEndEdit->setDisplayFormat("hh:mm");
        breakLayout->addWidget(m_dinnerBreakEndEdit, 3, 1);

        detailsLayout->addWidget(standardGroup);
        detailsLayout->addWidget(breakGroup);
        detailsGroup->setContentLayout(detailsLayout);




        // 结果显示
        QGroupBox* resultGroup = new QGroupBox(QString("计算结果"));
        QVBoxLayout* resultLayout = new QVBoxLayout(resultGroup);
        m_resultLabel = new QLabel(QString(""));
        m_resultLabel->setWordWrap(true);
        m_resultLabel->setStyleSheet("padding: 10px; background-color: #f0f0f0; border-radius: 5px;");
        resultLayout->addWidget(m_resultLabel);
        mainLayout->addWidget(resultGroup);
        mainLayout->addWidget(detailsGroup);

        // 按钮区域
        QHBoxLayout* buttonLayout = new QHBoxLayout();
        QPushButton* saveBtn = new QPushButton(QString("保存"));
        QPushButton* cancelBtn = new QPushButton(QString("取消"));

        connect(saveBtn, &QPushButton::clicked, this, &TimeSettingDialog::saveAndClose);
        connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

        buttonLayout->addStretch();
        buttonLayout->addWidget(saveBtn);
        buttonLayout->addWidget(cancelBtn);
        mainLayout->addLayout(buttonLayout);

        // 连接时间变化信号，自动重新计算
        connect(m_arrivalTimeEdit, &QTimeEdit::timeChanged, this, &TimeSettingDialog::calculateWorkTime);
        connect(m_departureTimeEdit, &QTimeEdit::timeChanged, this, &TimeSettingDialog::calculateWorkTime);
        connect(m_workStartTimeEdit, &QTimeEdit::timeChanged, this, &TimeSettingDialog::calculateWorkTime);
        connect(m_workEndTimeEdit, &QTimeEdit::timeChanged, this, &TimeSettingDialog::calculateWorkTime);
        connect(m_lunchBreakStartEdit, &QTimeEdit::timeChanged, this, &TimeSettingDialog::calculateWorkTime);
        connect(m_lunchBreakEndEdit, &QTimeEdit::timeChanged, this, &TimeSettingDialog::calculateWorkTime);
        connect(m_dinnerBreakStartEdit, &QTimeEdit::timeChanged, this, &TimeSettingDialog::calculateWorkTime);
        connect(m_dinnerBreakEndEdit, &QTimeEdit::timeChanged, this, &TimeSettingDialog::calculateWorkTime);
    }

    void loadRecord() {
        QSettings settings;
        QString key = m_date.toString("yyyy-MM-dd");

        m_arrivalTimeEdit->setTime(QTime::fromString(
            settings.value(key + "/arrival", "09:00").toString(), "hh:mm"));
        m_departureTimeEdit->setTime(QTime::fromString(
            settings.value(key + "/departure", "18:00").toString(), "hh:mm"));
        m_workStartTimeEdit->setTime(QTime::fromString(
            settings.value(key + "/workStart", "09:00").toString(), "hh:mm"));
        m_workEndTimeEdit->setTime(QTime::fromString(
            settings.value(key + "/workEnd", "18:00").toString(), "hh:mm"));
        m_lunchBreakStartEdit->setTime(QTime::fromString(
            settings.value(key + "/lunchStart", "12:30").toString(), "hh:mm"));
        m_lunchBreakEndEdit->setTime(QTime::fromString(
            settings.value(key + "/lunchEnd", "13:30").toString(), "hh:mm"));
        m_dinnerBreakStartEdit->setTime(QTime::fromString(
            settings.value(key + "/dinnerStart", "18:00").toString(), "hh:mm"));
        m_dinnerBreakEndEdit->setTime(QTime::fromString(
            settings.value(key + "/dinnerEnd", "18:30").toString(), "hh:mm"));
    }

    void saveRecord() {
        QSettings settings;
        QString key = m_date.toString("yyyy-MM-dd");

        settings.setValue(key + "/arrival", m_arrivalTimeEdit->time().toString("hh:mm"));
        settings.setValue(key + "/departure", m_departureTimeEdit->time().toString("hh:mm"));
        settings.setValue(key + "/workStart", m_workStartTimeEdit->time().toString("hh:mm"));
        settings.setValue(key + "/workEnd", m_workEndTimeEdit->time().toString("hh:mm"));
        settings.setValue(key + "/lunchStart", m_lunchBreakStartEdit->time().toString("hh:mm"));
        settings.setValue(key + "/lunchEnd", m_lunchBreakEndEdit->time().toString("hh:mm"));
        settings.setValue(key + "/dinnerStart", m_dinnerBreakStartEdit->time().toString("hh:mm"));
        settings.setValue(key + "/dinnerEnd", m_dinnerBreakEndEdit->time().toString("hh:mm"));
    }

private:
    QDate m_date;
    QTimeEdit* m_arrivalTimeEdit;
    QTimeEdit* m_departureTimeEdit;
    QTimeEdit* m_workStartTimeEdit;
    QTimeEdit* m_workEndTimeEdit;
    QTimeEdit* m_lunchBreakStartEdit;
    QTimeEdit* m_lunchBreakEndEdit;
    QTimeEdit* m_dinnerBreakStartEdit;
    QTimeEdit* m_dinnerBreakEndEdit;
    QLabel* m_resultLabel;
};

// 主窗口
class AttendanceMainWindow : public QMainWindow {
    Q_OBJECT

public:
    AttendanceMainWindow(QWidget* parent = nullptr) : QMainWindow(parent) {
        setWindowIcon(QIcon(":icon.ico"));
        setWindowTitle(QString("打卡管理系统"));
        setMinimumSize(800, 600);

        resize(900,660);

        setupUI();
        loadAttendanceData();
    }

private slots:
    void onDateClicked(const QDate& date) {
        TimeSettingDialog dialog(date, this);
        if (dialog.exec() == QDialog::Accepted) {
            updateCalendarAppearance();
            updateMonthlyStatistics();
        }
    }

    void onMonthChanged() {
        updateCalendarAppearance();
        updateMonthlyStatistics();
    }

private:
    void setupUI() {
        QWidget* centralWidget = new QWidget();
        setCentralWidget(centralWidget);

        QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);

        // 左侧：日历
        QVBoxLayout* leftLayout = new QVBoxLayout();

        QLabel* titleLabel = new QLabel(QString("打卡日历"));
        titleLabel->setAlignment(Qt::AlignCenter);
        titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; padding: 10px;");
        leftLayout->addWidget(titleLabel);

        m_calendar = new QCalendarWidget();
        m_calendar->setLocale(QLocale::Chinese);
        m_calendar->setFirstDayOfWeek(Qt::Monday);
        m_calendar->setGridVisible(true);
        leftLayout->addWidget(m_calendar);

        // 右侧：统计和工具
        QVBoxLayout* rightLayout = new QVBoxLayout();

        // 月度统计
        QGroupBox* statsGroup = new QGroupBox(QString("本月统计"));
        QVBoxLayout* statsLayout = new QVBoxLayout(statsGroup);
        m_statsLabel = new QLabel(QString("请选择日期查看统计"));
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
        connect(m_calendar, &QCalendarWidget::currentPageChanged,
                this, &AttendanceMainWindow::onMonthChanged);

        updateCalendarAppearance();
        updateMonthlyStatistics();
    }

    void loadAttendanceData() {
        // 数据已通过QSettings自动加载
    }

    void updateCalendarAppearance() {
        int year = m_calendar->yearShown();
        int month = m_calendar->monthShown();
        QDate startDate(year, month, 1);
        QDate endDate = startDate.addMonths(1).addDays(-1);

        QSettings settings;

        QDate date = startDate;
        while (date <= endDate) {
            QString key = date.toString("yyyy-MM-dd");

            if (settings.contains(key + "/arrival")) {
                // 有打卡记录的日期用绿色背景
                QTextCharFormat format;
                format.setBackground(QColor(144, 238, 144)); // 浅绿色
                m_calendar->setDateTextFormat(date, format);
            } else {
                // 清除格式
                m_calendar->setDateTextFormat(date, QTextCharFormat());
            }
            date = date.addDays(1);
        }
    }

    void updateMonthlyStatistics() {
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

                // 创建记录并计算
                AttendanceRecord record;
                record.arrivalTime = QTime::fromString(settings.value(key + "/arrival").toString(), "hh:mm");
                record.departureTime = QTime::fromString(settings.value(key + "/departure").toString(), "hh:mm");
                record.workStartTime = QTime::fromString(settings.value(key + "/workStart", "09:00").toString(), "hh:mm");
                record.workEndTime = QTime::fromString(settings.value(key + "/workEnd", "18:00").toString(), "hh:mm");
                record.lunchBreakStart = QTime::fromString(settings.value(key + "/lunchStart", "12:30").toString(), "hh:mm");
                record.lunchBreakEnd = QTime::fromString(settings.value(key + "/lunchEnd", "13:30").toString(), "hh:mm");
                record.dinnerBreakStart = QTime::fromString(settings.value(key + "/dinnerStart", "18:00").toString(), "hh:mm");
                record.dinnerBreakEnd = QTime::fromString(settings.value(key + "/dinnerEnd", "18:30").toString(), "hh:mm");

                // 计算工作时间结果
                WorkTimeResult result = WorkTimeCalculator::calculateWorkTimeResult(record);

                if (result.overtimeMinutes > 0) {
                    totalOvertimeMinutes += result.overtimeMinutes;
                }
                totalLateMinutes += result.lateMinutes;
                totalEarlyLeaveMinutes += result.earlyLeaveMinutes;
            }
            date = date.addDays(1);
        }

        QString stats = QString("统计月份: %1年%2月\n")
                            .arg(year)
                            .arg(month);
        stats += QString("打卡天数: %1天\n").arg(workDays);
        stats += QString("总加班时间: %1小时%2分钟\n")
                     .arg(totalOvertimeMinutes / 60)
                     .arg(totalOvertimeMinutes % 60);
        stats += QString("总迟到时间: %1小时%2分钟\n")
                     .arg(totalLateMinutes / 60)
                     .arg(totalLateMinutes % 60);
        stats += QString("总早退时间: %1小时%2分钟")
                     .arg(totalEarlyLeaveMinutes / 60)
                     .arg(totalEarlyLeaveMinutes % 60);

        m_statsLabel->setText(stats);
    }

private:
    QCalendarWidget* m_calendar;
    QLabel* m_statsLabel;
};

#include "main.moc"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    // Qt5的编码设置
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
#endif
    // 在main.cpp中设置应用图标



    // 设置应用程序信息
    app.setApplicationName("AttendanceApp");
    app.setOrganizationName("MyCompany");

    // 设置中文字体
    QFont font = app.font();
    font.setFamily("Microsoft YaHei");
    font.setPointSize(9);
    app.setFont(font);

    AttendanceMainWindow window;
    window.show();

    return app.exec();
}
