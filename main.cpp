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
#pragma execution_character_set("utf-8")
#if defined(_MSC_VER) && (_MSC_VER >= 1600)
# pragma execution_character_set("utf-8")
#endif
// 确保UTF-8编码支持
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#pragma execution_character_set("utf-8")
#endif

// 打卡记录结构体
struct AttendanceRecord {
    QTime arrivalTime;      // 到公司时间
    QTime departureTime;    // 出公司时间
    QTime workStartTime;    // 实际上班时间
    QTime workEndTime;      // 实际下班时间

    AttendanceRecord() {
        arrivalTime = QTime(9, 0);
        departureTime = QTime(18, 0);
        workStartTime = QTime(9, 0);
        workEndTime = QTime(18, 0);
    }
};

// 时间设置对话框
class TimeSettingDialog : public QDialog {
    Q_OBJECT

public:
    TimeSettingDialog(const QDate& date, QWidget* parent = nullptr)
        : QDialog(parent), m_date(date) {
        // setWindowTitle(QString("设置打卡时间 - %1").arg(date.toString(QString("yyyy年M月d日"))));
        setModal(true);
        resize(400, 300);

        setupUI();
        loadRecord();
    }

    AttendanceRecord getRecord() const {
        AttendanceRecord record;
        record.arrivalTime = m_arrivalTimeEdit->time();
        record.departureTime = m_departureTimeEdit->time();
        record.workStartTime = m_workStartTimeEdit->time();
        record.workEndTime = m_workEndTimeEdit->time();
        return record;
    }

private slots:
    void calculateOvertime() {
        AttendanceRecord record = getRecord();

        // 计算实际工作时间（分钟）
        int actualWorkMinutes = record.arrivalTime.secsTo(record.departureTime) / 60;

        // 计算标准工作时间（分钟）
        int standardWorkMinutes = record.workStartTime.secsTo(record.workEndTime) / 60;

        // 计算加班时间
        int overtimeMinutes = actualWorkMinutes - standardWorkMinutes;

        QString result;
        result += QString("实际工作时间: %1小时%2分钟\n")
                      .arg(actualWorkMinutes / 60)
                      .arg(actualWorkMinutes % 60);
        result += QString("标准工作时间: %1小时%2分钟\n")
                      .arg(standardWorkMinutes / 60)
                      .arg(standardWorkMinutes % 60);

        if (overtimeMinutes > 0) {
            result += QString("加班时间: %1小时%2分钟")
                          .arg(overtimeMinutes / 60)
                          .arg(overtimeMinutes % 60);
        } else if (overtimeMinutes < 0) {
            result += QString("早退时间: %1小时%2分钟")
                          .arg((-overtimeMinutes) / 60)
                          .arg((-overtimeMinutes) % 60);
        } else {
            result += QString("正常下班，无加班");
        }

        m_resultLabel->setText(result);
    }

    void saveAndClose() {
        saveRecord();
        accept();
    }

private:
    void setupUI() {
        QVBoxLayout* mainLayout = new QVBoxLayout(this);

        // 时间设置区域
        QGroupBox* timeGroup = new QGroupBox(QString("时间设置"));
        QGridLayout* timeLayout = new QGridLayout(timeGroup);

        timeLayout->addWidget(new QLabel(QString("到公司时间:")), 0, 0);
        m_arrivalTimeEdit = new QTimeEdit();
        m_arrivalTimeEdit->setDisplayFormat("hh:mm");
        timeLayout->addWidget(m_arrivalTimeEdit, 0, 1);

        timeLayout->addWidget(new QLabel(QString("出公司时间:")), 1, 0);
        m_departureTimeEdit = new QTimeEdit();
        m_departureTimeEdit->setDisplayFormat("hh:mm");
        timeLayout->addWidget(m_departureTimeEdit, 1, 1);

        timeLayout->addWidget(new QLabel(QString("标准上班时间:")), 2, 0);
        m_workStartTimeEdit = new QTimeEdit();
        m_workStartTimeEdit->setDisplayFormat("hh:mm");
        timeLayout->addWidget(m_workStartTimeEdit, 2, 1);

        timeLayout->addWidget(new QLabel(QString("标准下班时间:")), 3, 0);
        m_workEndTimeEdit = new QTimeEdit();
        m_workEndTimeEdit->setDisplayFormat("hh:mm");
        timeLayout->addWidget(m_workEndTimeEdit, 3, 1);

        mainLayout->addWidget(timeGroup);

        // 计算按钮
        QPushButton* calculateBtn = new QPushButton(QString("计算加班时间"));
        connect(calculateBtn, &QPushButton::clicked, this, &TimeSettingDialog::calculateOvertime);
        mainLayout->addWidget(calculateBtn);

        // 结果显示
        QGroupBox* resultGroup = new QGroupBox(QString("计算结果"));
        QVBoxLayout* resultLayout = new QVBoxLayout(resultGroup);
        m_resultLabel = new QLabel(QString(""));//请点击计算按钮
        m_resultLabel->setWordWrap(true);
        m_resultLabel->setStyleSheet("padding: 10px; background-color: #f0f0f0; border-radius: 5px;");
        resultLayout->addWidget(m_resultLabel);
        mainLayout->addWidget(resultGroup);

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
        connect(m_arrivalTimeEdit, &QTimeEdit::timeChanged, this, &TimeSettingDialog::calculateOvertime);
        connect(m_departureTimeEdit, &QTimeEdit::timeChanged, this, &TimeSettingDialog::calculateOvertime);
        connect(m_workStartTimeEdit, &QTimeEdit::timeChanged, this, &TimeSettingDialog::calculateOvertime);
        connect(m_workEndTimeEdit, &QTimeEdit::timeChanged, this, &TimeSettingDialog::calculateOvertime);
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
    }

    void saveRecord() {
        QSettings settings;
        QString key = m_date.toString("yyyy-MM-dd");

        settings.setValue(key + "/arrival", m_arrivalTimeEdit->time().toString("hh:mm"));
        settings.setValue(key + "/departure", m_departureTimeEdit->time().toString("hh:mm"));
        settings.setValue(key + "/workStart", m_workStartTimeEdit->time().toString("hh:mm"));
        settings.setValue(key + "/workEnd", m_workEndTimeEdit->time().toString("hh:mm"));
    }

private:
    QDate m_date;
    QTimeEdit* m_arrivalTimeEdit;
    QTimeEdit* m_departureTimeEdit;
    QTimeEdit* m_workStartTimeEdit;
    QTimeEdit* m_workEndTimeEdit;
    QLabel* m_resultLabel;
};


// 主窗口
class AttendanceMainWindow : public QMainWindow {
    Q_OBJECT

public:
    AttendanceMainWindow(QWidget* parent = nullptr) : QMainWindow(parent) {
        setWindowTitle(QString("打卡管理系统"));
        setMinimumSize(800, 600);

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

        QDate date = startDate;
        while (date <= endDate) {
            QString key = date.toString("yyyy-MM-dd");

            if (settings.contains(key + "/arrival")) {
                workDays++;

                QTime arrival = QTime::fromString(settings.value(key + "/arrival").toString(), "hh:mm");
                QTime departure = QTime::fromString(settings.value(key + "/departure").toString(), "hh:mm");
                QTime workStart = QTime::fromString(settings.value(key + "/workStart").toString(), "hh:mm");
                QTime workEnd = QTime::fromString(settings.value(key + "/workEnd").toString(), "hh:mm");

                int actualMinutes = arrival.secsTo(departure) / 60;
                int standardMinutes = workStart.secsTo(workEnd) / 60;
                int overtime = actualMinutes - standardMinutes;

                if (overtime > 0) {
                    totalOvertimeMinutes += overtime;
                }
            }
            date = date.addDays(1);
        }

        QString stats = QString("统计月份: %1年%2月\n")
                            .arg(year)
                            .arg(month);
        stats += QString("打卡天数: %1天\n").arg(workDays);
        stats += QString("总加班时间: %1小时%2分钟")
                     .arg(totalOvertimeMinutes / 60)
                     .arg(totalOvertimeMinutes % 60);

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
