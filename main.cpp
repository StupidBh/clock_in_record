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
#include <QMenu>
#include <QAction>
#include <QContextMenuEvent>
#include <QMouseEvent>
#pragma execution_character_set("utf-8")
#if defined(_MSC_VER) && (_MSC_VER >= 1600)
# pragma execution_character_set("utf-8")
#endif

// 自定义可折叠的分组
class CollapsibleGroupBox : public QWidget {
    Q_OBJECT // 需要Q_OBJECT宏来支持信号槽

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
    QTime arrivalTime;      // 到达公司时间
    QTime departureTime;    // 离开公司时间
    QTime workStartTime;    // 标准上班时间
    QTime workEndTime;      // 标准下班时间
    QTime lunchBreakStart;  // 午餐开始时间
    QTime lunchBreakEnd;    // 午餐结束时间
    QTime dinnerBreakStart; // 晚餐开始时间
    QTime dinnerBreakEnd;   // 晚餐结束时间

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

        // 午餐时间
        if (isTimeRangeOverlap(record.arrivalTime, record.departureTime,
                               record.lunchBreakStart, record.lunchBreakEnd)) {
            QTime lunchStart = maxTime(record.arrivalTime, record.lunchBreakStart);
            QTime lunchEnd = minTime(record.departureTime, record.lunchBreakEnd);
            if (lunchStart < lunchEnd) {
                result.totalBreakMinutes += lunchStart.secsTo(lunchEnd) / 60;
            }
        }

        // 晚餐时间
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

        // 标准午餐时间
        if (record.lunchBreakStart < record.lunchBreakEnd) {
            standardBreakMinutes += record.lunchBreakStart.secsTo(record.lunchBreakEnd) / 60;
        }

        // 标准晚餐时间（如果在工作时间内）
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

        // 显示迟到早退
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

        // 显示加班或欠时
        if (result.overtimeMinutes > 0) {
            resultText += QString("[加班时间] %1小时%2分钟")
                              .arg(result.overtimeMinutes / 60)
                              .arg(result.overtimeMinutes % 60);
        } else if (result.overtimeMinutes < 0) {
            resultText += QString("[欠缺时间] %1小时%2分钟")
                              .arg((-result.overtimeMinutes) / 60)
                              .arg((-result.overtimeMinutes) % 60);
        } else {
            resultText += QString("[完成标准时间]");
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

        breakLayout->addWidget(new QLabel(QString("午餐开始时间:")), 0, 0);
        m_lunchBreakStartEdit = new QTimeEdit();
        m_lunchBreakStartEdit->setDisplayFormat("hh:mm");
        breakLayout->addWidget(m_lunchBreakStartEdit, 0, 1);

        breakLayout->addWidget(new QLabel(QString("午餐结束时间:")), 1, 0);
        m_lunchBreakEndEdit = new QTimeEdit();
        m_lunchBreakEndEdit->setDisplayFormat("hh:mm");
        breakLayout->addWidget(m_lunchBreakEndEdit, 1, 1);

        breakLayout->addWidget(new QLabel(QString("晚餐开始时间:")), 2, 0);
        m_dinnerBreakStartEdit = new QTimeEdit();
        m_dinnerBreakStartEdit->setDisplayFormat("hh:mm");
        breakLayout->addWidget(m_dinnerBreakStartEdit, 2, 1);

        breakLayout->addWidget(new QLabel(QString("晚餐结束时间:")), 3, 0);
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

        // 监听时间变化信号，自动更新计算
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

        QTime defaultTime = QTime::currentTime();
        QTime arrivalTime =
            QTime::fromString(settings.value(key + "/arrival", defaultTime.toString("hh:mm")).toString(), "hh:mm");
        m_arrivalTimeEdit->setTime(arrivalTime);

        m_departureTimeEdit->setTime(
            QTime::fromString(settings.value(key + "/departure", "21:01").toString(), "hh:mm"));

        m_workStartTimeEdit->setTime(
            QTime::fromString(settings.value(key + "/workStart", "09:00").toString(), "hh:mm"));
        m_workEndTimeEdit->setTime(QTime::fromString(settings.value(key + "/workEnd", "18:00").toString(), "hh:mm"));
        m_lunchBreakStartEdit->setTime(
            QTime::fromString(settings.value(key + "/lunchStart", "12:30").toString(), "hh:mm"));
        m_lunchBreakEndEdit->setTime(QTime::fromString(settings.value(key + "/lunchEnd", "13:30").toString(), "hh:mm"));
        m_dinnerBreakStartEdit->setTime(
            QTime::fromString(settings.value(key + "/dinnerStart", "18:00").toString(), "hh:mm"));
        m_dinnerBreakEndEdit->setTime(
            QTime::fromString(settings.value(key + "/dinnerEnd", "18:30").toString(), "hh:mm"));
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

// 自定义日历控件，支持右键菜单
class CustomCalendarWidget : public QCalendarWidget {
    Q_OBJECT
private:
    QTableView* m_tableView{nullptr};
public:
    CustomCalendarWidget(QWidget* parent = nullptr) : QCalendarWidget(parent) {
        setContextMenuPolicy(Qt::CustomContextMenu);
        connect(this, &QWidget::customContextMenuRequested, this, &CustomCalendarWidget::showContextMenu);
        setupEventFilters();
    }
    void setupEventFilters()
    {
        m_tableView = this->findChild<QTableView*>();
        if (m_tableView) {
            // 只监听右键，双击用重写的方法处理
            m_tableView->installEventFilter(this);
        }
    }
signals:
    void deleteRequested(const QDate& date);

private slots:
    void showContextMenu(const QPoint& pos) {
        // 获取点击位置对应的日期
        QDate clickedDate = dateAt(pos);
        if (!clickedDate.isValid()) {
            return;
        }

        // 检查该日期是否有记录
        QSettings settings;
        QString key = clickedDate.toString("yyyy-MM-dd");
        if (!settings.contains(key + "/arrival")) {
            return; // 没有记录，不显示菜单
        }

        // 创建右键菜单
        QMenu contextMenu(this);
        QAction* deleteAction = contextMenu.addAction(QString("删除 %1 的记录").arg(clickedDate.toString("yyyy-MM-dd")));
        deleteAction->setIcon(style()->standardIcon(QStyle::SP_TrashIcon));

        // 显示菜单并处理选择
        QAction* selectedAction = contextMenu.exec(mapToGlobal(pos));
        if (selectedAction == deleteAction) {
            emit deleteRequested(clickedDate);
        }
    }

private:
    QDate dateAt(const QPoint& pos) {
        // 这是一个简化的实现，在实际使用中可能需要更精确的计算
        // 使用selectedDate作为近似值
        return getDateFromPosition(QPoint(pos.x(),pos.y()-20));
    }
    QDate getDateFromPosition(const QPoint &pos)
    {
        if (!m_tableView) {
            return QDate();
        }

        QModelIndex index = m_tableView->indexAt(pos);
        if (!index.isValid()) {
            return QDate();
        }

        // 获取模型
        QAbstractItemModel *model = m_tableView->model();
        if (!model) {
            return QDate();
        }

        // 尝试不同的角色获取日期数据
        QVariant dateData;

        // 尝试 Qt::UserRole
        dateData = model->data(index, Qt::UserRole);
        if (dateData.canConvert<QDate>()) {
            return dateData.toDate();
        }

        // 尝试 Qt::UserRole + 1
        dateData = model->data(index, Qt::UserRole + 1);
        if (dateData.canConvert<QDate>()) {
            return dateData.toDate();
        }

        // 如果都获取不到，使用改进的计算方法
        return calculateDateFromRowCol(index.row(), index.column());
    }

    QDate calculateDateFromRowCol(int row, int col)
    {
        // 获取当前显示的年月
        int year = yearShown();
        int month = monthShown();
        QDate firstDay(year, month, 1);

        // 获取日历的第一天设置
        Qt::DayOfWeek startDay = firstDayOfWeek();

        // 计算第一天在表格中的位置
        int firstDayColumn;
        if (startDay == Qt::Sunday) {
            firstDayColumn = firstDay.dayOfWeek() % 7; // Sunday = 0
        } else {
            firstDayColumn = firstDay.dayOfWeek() - 1; // Monday = 0
        }

        // 计算当前位置对应的天数
        int totalCells = (row-1) * 7 + col -1;
        int dayNumber = totalCells - firstDayColumn + 1;

        if (dayNumber <= 0) {
            // 上个月的日期
            QDate prevMonth = firstDay.addMonths(-1);
            return QDate(prevMonth.year(), prevMonth.month(),
                         prevMonth.daysInMonth() + dayNumber);
        } else if (dayNumber > firstDay.daysInMonth()) {
            // 下个月的日期
            QDate nextMonth = firstDay.addMonths(1);
            return QDate(nextMonth.year(), nextMonth.month(),
                         dayNumber - firstDay.daysInMonth());
        } else {
            // 当前月的日期
            return QDate(year, month, dayNumber);
        }
    }


};

// 主窗口
class AttendanceMainWindow : public QMainWindow {
    Q_OBJECT

public:
    AttendanceMainWindow(QWidget* parent = nullptr) : QMainWindow(parent) {

        setWindowTitle(QString("打卡管理系统"));
        setMinimumSize(800, 600);

        resize(900,660);

        setupUI();
        loadAttendanceData();
    }

protected:
    void mousePressEvent(QMouseEvent* event) override {
        // 检查点击位置是否在日历区域外
        if (m_calendar) {
            QPoint calendarPos = m_calendar->mapFromGlobal(event->globalPos());
            QRect calendarRect = m_calendar->rect();

            // 如果点击在日历外，重置选择状态
            if (!calendarRect.contains(calendarPos)) {
                // 将选择重置为看不见的日期，并将页面调回当前页面，这样看起来像失去焦点
                // m_calendar->setSelectedDate(QDate::currentDate());
                m_calendar->setSelectedDate(QDate::currentDate().addDays(365));
                m_calendar->setCurrentPage(QDate::currentDate().year(),QDate::currentDate().month());
            }
        }

        QMainWindow::mousePressEvent(event);
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

    void onDeleteRequested(const QDate& date) {
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

private:
    void setupUI() {
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
        QLabel* helpLabel = new QLabel(QString("使用说明：\n? 左键点击日期设置考勤时间\n? 右键点击有记录的日期可删除记录\n? 点击日历外区域可重置选择状态"));
        helpLabel->setStyleSheet("color: #666; font-size: 12px; padding: 10px; background-color: #f5f5f5; border-radius: 5px;");
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
        connect(m_calendar, &CustomCalendarWidget::deleteRequested,
                this, &AttendanceMainWindow::onDeleteRequested);

        updateCalendarAppearance();
        updateMonthlyStatistics();
    }

    void loadAttendanceData() {
        // 数据通过QSettings自动加载
    }

    void deleteAttendanceRecord(const QDate& date) {
        QSettings settings;
        QString key = date.toString("yyyy-MM-dd");

        // 删除所有相关的设置项
        QStringList keys = {
            key + "/arrival",
            key + "/departure",
            key + "/workStart",
            key + "/workEnd",
            key + "/lunchStart",
            key + "/lunchEnd",
            key + "/dinnerStart",
            key + "/dinnerEnd"
        };

        for (const QString& k : keys) {
            settings.remove(k);
        }

        // 更新界面
        updateCalendarAppearance();
        updateMonthlyStatistics();

        // 显示删除成功消息
        QMessageBox::information(this, QString("删除成功"),
                                 QString("已成功删除 %1 的考勤记录").arg(date.toString("yyyy-MM-dd")));
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
                // 有打卡记录，显示绿色背景
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

                // 加载记录并计算
                AttendanceRecord record;
                record.arrivalTime = QTime::fromString(settings.value(key + "/arrival").toString(), "hh:mm");
                record.departureTime = QTime::fromString(settings.value(key + "/departure").toString(), "hh:mm");
                record.workStartTime = QTime::fromString(settings.value(key + "/workStart", "09:00").toString(), "hh:mm");
                record.workEndTime = QTime::fromString(settings.value(key + "/workEnd", "18:00").toString(), "hh:mm");
                record.lunchBreakStart = QTime::fromString(settings.value(key + "/lunchStart", "12:30").toString(), "hh:mm");
                record.lunchBreakEnd = QTime::fromString(settings.value(key + "/lunchEnd", "13:30").toString(), "hh:mm");
                record.dinnerBreakStart = QTime::fromString(settings.value(key + "/dinnerStart", "18:00").toString(), "hh:mm");
                record.dinnerBreakEnd = QTime::fromString(settings.value(key + "/dinnerEnd", "18:30").toString(), "hh:mm");

                // 计算工作时间数据
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
        stats += QString("工作天数: %1天\n").arg(workDays);
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
    CustomCalendarWidget* m_calendar;
    QLabel* m_statsLabel;
};



int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    // Qt5的编码设置
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
#endif
    // 在main.cpp中添加应用图标

    app.setWindowIcon(QIcon(":icon.ico"));

    // 设置应用程序信息
    app.setApplicationName("AttendanceApp");
    app.setOrganizationName("MyCompany");

    // 设置默认字体
    QFont font = app.font();
    font.setFamily("Microsoft YaHei");
    font.setPointSize(9);
    app.setFont(font);

    AttendanceMainWindow window;
    window.show();

    return app.exec();
}
#include "main.moc"
