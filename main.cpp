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
    #pragma execution_character_set("utf-8")
#endif

// �Զ�����۵��ķ���
class CollapsibleGroupBox : public QWidget {
Q_OBJECT // ��ҪQ_OBJECT����֧���źŲ�

    public :
    CollapsibleGroupBox(const QString& title, QWidget* parent = nullptr) :
    QWidget(parent),
    m_collapsed(true),
    m_title(title)
    {
        m_toggleButton = new QPushButton();
        m_toggleButton->setCheckable(true);
        m_toggleButton->setChecked(false);
        m_toggleButton->setStyleSheet(
            "QPushButton { text-align: left; border: none; font-weight: bold; padding: 5px; }"
            "QPushButton:checked { background-color: #e0e0e0; }");

        m_contentWidget = new QWidget();
        m_contentWidget->setWindowFlags(Qt::Popup);
        m_contentWidget->setVisible(false);

        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->addWidget(m_toggleButton);
        // layout->addWidget(m_contentWidget);
        layout->setContentsMargins(0, 0, 0, 0);

        connect(m_toggleButton, &QPushButton::toggled, this, &CollapsibleGroupBox::toggle);

        updateButtonText();
    }

    void setContentLayout(QLayout* layout)
    {
        m_contentWidget->setLayout(layout);
        auto pw = this->parentWidget();
        if (pw) {
            pw->resize(pw->size().width(), pw->sizeHint().height());
        }
    }

    QWidget* contentWidget() const { return m_contentWidget; }

private slots:

    void toggle(bool checked)
    {
        m_collapsed = !checked;
        m_contentWidget->setVisible(checked);
        QPoint globalPos = m_toggleButton->mapToGlobal(QPoint(0, 0));
        int x = globalPos.x();
        int y = globalPos.y() + m_toggleButton->height(); // ��ʾ�ڰ�ť�·�
        m_contentWidget->move(x, y);
        updateButtonText();
    }

private:
    void updateButtonText()
    {
        QString arrow = m_collapsed ? ">" : "v";
        m_toggleButton->setText(arrow + " " + m_title);
    }

    QPushButton* m_toggleButton;
    QWidget* m_contentWidget;
    bool m_collapsed;
    QString m_title;
};

// �򿨼�¼�ṹ��
struct AttendanceRecord {
    bool needAverageCal;    // 是否加入平均加班计算
    QTime arrivalTime;      // ���﹫˾ʱ��
    QTime departureTime;    // �뿪��˾ʱ��
    QTime workStartTime;    // ��׼�ϰ�ʱ��
    QTime workEndTime;      // ��׼�°�ʱ��
    QTime lunchBreakStart;  // ��Ϳ�ʼʱ��
    QTime lunchBreakEnd;    // ��ͽ���ʱ��
    QTime dinnerBreakStart; // ��Ϳ�ʼʱ��
    QTime dinnerBreakEnd;   // ��ͽ���ʱ��

    AttendanceRecord()
    {
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

// ����ʱ�������
struct WorkTimeResult
{
    int actualWorkMinutes = 0;   // ʵ�ʹ���ʱ�䣨���ӣ�
    int standardWorkMinutes = 0; // ��׼����ʱ�䣨���ӣ�
    int lateMinutes = 0;         // �ٵ�ʱ�䣨���ӣ�
    int lateSize = 0;
    int earlyLeaveMinutes = 0;   // ����ʱ�䣨���ӣ�
    int earlyLeaveSize = 0;
    int overtimeMinutes = 0;     // �Ӱ�ʱ�䣨���ӣ�
    int totalBreakMinutes = 0;   // ����Ϣʱ�䣨���ӣ�
};

// ����ʱ����㹤����
class WorkTimeCalculator {
public:
    static WorkTimeResult calculateWorkTimeResult(const AttendanceRecord& record)
    {
        WorkTimeResult result;

        // ����ٵ�ʱ��
        if (record.arrivalTime > record.workStartTime) {
            result.lateMinutes = record.workStartTime.secsTo(record.arrivalTime) / 60;
        }

        // ��������ʱ��
        if (record.departureTime < record.workEndTime) {
            result.earlyLeaveMinutes = record.departureTime.secsTo(record.workEndTime) / 60;
        }

        // �����ڹ�˾��ʱ��
        int totalMinutesAtWork = record.arrivalTime.secsTo(record.departureTime) / 60;

        // ����ʵ����Ϣʱ��
        result.totalBreakMinutes = 0;

        // ���ʱ��
        if (isTimeRangeOverlap(
                record.arrivalTime,
                record.departureTime,
                record.lunchBreakStart,
                record.lunchBreakEnd)) {
            QTime lunchStart = maxTime(record.arrivalTime, record.lunchBreakStart);
            QTime lunchEnd = minTime(record.departureTime, record.lunchBreakEnd);
            if (lunchStart < lunchEnd) {
                result.totalBreakMinutes += lunchStart.secsTo(lunchEnd) / 60;
            }
        }

        // ���ʱ��
        if (isTimeRangeOverlap(
                record.arrivalTime,
                record.departureTime,
                record.dinnerBreakStart,
                record.dinnerBreakEnd)) {
            QTime dinnerStart = maxTime(record.arrivalTime, record.dinnerBreakStart);
            QTime dinnerEnd = minTime(record.departureTime, record.dinnerBreakEnd);
            if (dinnerStart < dinnerEnd) {
                result.totalBreakMinutes += dinnerStart.secsTo(dinnerEnd) / 60;
            }
        }

        // ʵ�ʹ���ʱ�� = �ڹ�˾ʱ�� - ��Ϣʱ��
        result.actualWorkMinutes = totalMinutesAtWork - result.totalBreakMinutes;

        // ��׼����ʱ��
        int standardTotalMinutes = record.workStartTime.secsTo(record.workEndTime) / 60;
        int standardBreakMinutes = 0;

        // ��׼���ʱ��
        if (record.lunchBreakStart>=record.workStartTime &&
            record.lunchBreakStart<record.workEndTime&&
            record.lunchBreakStart < record.lunchBreakEnd) {
            standardBreakMinutes += record.lunchBreakStart.secsTo(record.lunchBreakEnd) / 60;
        }
        //qDebug()<<standardBreakMinutes;
        // ��׼���ʱ�䣨����ڹ���ʱ���ڣ�
        if (record.dinnerBreakStart >= record.workStartTime && record.dinnerBreakStart < record.workEndTime) {
            QTime dinnerEnd = minTime(record.dinnerBreakEnd, record.workEndTime);
            if (record.dinnerBreakStart < dinnerEnd) {
                standardBreakMinutes += record.dinnerBreakStart.secsTo(dinnerEnd) / 60;
            }
        }



        result.standardWorkMinutes = standardTotalMinutes - standardBreakMinutes;

        // �Ӱ�ʱ��
        result.overtimeMinutes = result.actualWorkMinutes - result.standardWorkMinutes;

        return result;
    }

private:
    // ��������
    static bool isTimeRangeOverlap(const QTime& start1, const QTime& end1, const QTime& start2, const QTime& end2)
    {
        return start1 < end2 && start2 < end1;
    }

    static QTime maxTime(const QTime& time1, const QTime& time2) { return time1 > time2 ? time1 : time2; }

    static QTime minTime(const QTime& time1, const QTime& time2) { return time1 < time2 ? time1 : time2; }
};

// ʱ�����öԻ���
class TimeSettingDialog : public QDialog {
    Q_OBJECT

public:
    TimeSettingDialog(const QDate& date, QWidget* parent = nullptr) :
        QDialog(parent),
        m_date(date)
    {
        setWindowTitle(QString("���ô�ʱ�� - %1").arg(date.toString("yyyy-MM-dd")));
        setModal(true);
        resize(450, 400);

        setupUI();
        loadRecord();
    }

    AttendanceRecord getRecord() const
    {
        AttendanceRecord record;
        record.needAverageCal = m_needAverageCalCheckBox->isChecked();
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

    void calculateWorkTime()
    {
        AttendanceRecord record = getRecord();
        WorkTimeResult result = WorkTimeCalculator::calculateWorkTimeResult(record);

        QString resultText;

        // ��ʾ�ٵ�����
        if (result.lateMinutes > 0) {
            resultText += QString("[�ٵ�] %1Сʱ%2����\n").arg(result.lateMinutes / 60).arg(result.lateMinutes % 60);
        }

        if (result.earlyLeaveMinutes > 0) {
            resultText +=
                QString("[����] %1Сʱ%2����\n").arg(result.earlyLeaveMinutes / 60).arg(result.earlyLeaveMinutes % 60);
        }

        // ��ʾ����ʱ��
        resultText +=
            QString("[ʵ�ʹ���] %1Сʱ%2����\n").arg(result.actualWorkMinutes / 60).arg(result.actualWorkMinutes % 60);

        resultText += QString("[��׼����] %1Сʱ%2����\n")
                          .arg(result.standardWorkMinutes / 60)
                          .arg(result.standardWorkMinutes % 60);

        resultText +=
            QString("[����Ϣ] %1Сʱ%2����\n").arg(result.totalBreakMinutes / 60).arg(result.totalBreakMinutes % 60);

        // ��ʾ�Ӱ��Ƿʱ
        if (result.overtimeMinutes > 0) {
            resultText +=
                QString("[�Ӱ�ʱ��] %1Сʱ%2����").arg(result.overtimeMinutes / 60).arg(result.overtimeMinutes % 60);
        }
        else if (result.overtimeMinutes < 0) {
            resultText += QString("[Ƿȱʱ��] %1Сʱ%2����")
                              .arg((-result.overtimeMinutes) / 60)
                              .arg((-result.overtimeMinutes) % 60);
        }
        else {
            resultText += QString("[��ɱ�׼ʱ��]");
        }

        m_resultLabel->setText(resultText);
    }

    void saveAndClose()
    {
        saveRecord();
        accept();
    }

private:
    void setupUI()
    {
        QVBoxLayout* mainLayout = new QVBoxLayout(this);

        // ����ʱ��������
        QGroupBox* basicTimeGroup = new QGroupBox(QString("����ʱ��"));
        QGridLayout* basicTimeLayout = new QGridLayout(basicTimeGroup);

        basicTimeLayout->addWidget(new QLabel(QString("���﹫˾ʱ��:")), 0, 0);
        m_arrivalTimeEdit = new QTimeEdit();
        m_arrivalTimeEdit->setDisplayFormat("hh:mm");
        basicTimeLayout->addWidget(m_arrivalTimeEdit, 0, 1);

        basicTimeLayout->addWidget(new QLabel(QString("�뿪��˾ʱ��:")), 1, 0);
        m_departureTimeEdit = new QTimeEdit();
        m_departureTimeEdit->setDisplayFormat("hh:mm");
        basicTimeLayout->addWidget(m_departureTimeEdit, 1, 1);

        mainLayout->addWidget(basicTimeGroup);

        // ���۵�����ϸ����
        CollapsibleGroupBox* detailsGroup = new CollapsibleGroupBox(QString("��ϸ����"), this);

        QVBoxLayout* detailsLayout = new QVBoxLayout();
        // ����ƽ���Ӱ�ʱ��ѡ���
        QGroupBox* needAverageGroup = new QGroupBox(QString(""));
        QGridLayout* needAverageLayout = new QGridLayout(needAverageGroup);
        m_needAverageCalCheckBox = new QCheckBox("����ƽ���Ӱ���㣺");
        m_needAverageCalCheckBox->setLayoutDirection(Qt::RightToLeft);
        m_needAverageCalCheckBox->setChecked(true);
        needAverageLayout->addWidget(m_needAverageCalCheckBox);


        // ��׼����ʱ��
        QGroupBox* standardGroup = new QGroupBox(QString("��׼����ʱ��"));
        QGridLayout* standardLayout = new QGridLayout(standardGroup);

        standardLayout->addWidget(new QLabel(QString("��׼�ϰ�ʱ��:")), 0, 0);
        m_workStartTimeEdit = new QTimeEdit();
        m_workStartTimeEdit->setDisplayFormat("hh:mm");
        standardLayout->addWidget(m_workStartTimeEdit, 0, 1);

        standardLayout->addWidget(new QLabel(QString("��׼�°�ʱ��:")), 1, 0);
        m_workEndTimeEdit = new QTimeEdit();
        m_workEndTimeEdit->setDisplayFormat("hh:mm");
        standardLayout->addWidget(m_workEndTimeEdit, 1, 1);

        // ��Ϣʱ������
        QGroupBox* breakGroup = new QGroupBox(QString("��Ϣʱ������"));
        QGridLayout* breakLayout = new QGridLayout(breakGroup);

        breakLayout->addWidget(new QLabel(QString("��Ϳ�ʼʱ��:")), 0, 0);
        m_lunchBreakStartEdit = new QTimeEdit();
        m_lunchBreakStartEdit->setDisplayFormat("hh:mm");
        breakLayout->addWidget(m_lunchBreakStartEdit, 0, 1);

        breakLayout->addWidget(new QLabel(QString("��ͽ���ʱ��:")), 1, 0);
        m_lunchBreakEndEdit = new QTimeEdit();
        m_lunchBreakEndEdit->setDisplayFormat("hh:mm");
        breakLayout->addWidget(m_lunchBreakEndEdit, 1, 1);

        breakLayout->addWidget(new QLabel(QString("��Ϳ�ʼʱ��:")), 2, 0);
        m_dinnerBreakStartEdit = new QTimeEdit();
        m_dinnerBreakStartEdit->setDisplayFormat("hh:mm");
        breakLayout->addWidget(m_dinnerBreakStartEdit, 2, 1);

        breakLayout->addWidget(new QLabel(QString("��ͽ���ʱ��:")), 3, 0);
        m_dinnerBreakEndEdit = new QTimeEdit();
        m_dinnerBreakEndEdit->setDisplayFormat("hh:mm");
        breakLayout->addWidget(m_dinnerBreakEndEdit, 3, 1);

        detailsLayout->addWidget(needAverageGroup);
        detailsLayout->addWidget(standardGroup);
        detailsLayout->addWidget(breakGroup);
        detailsGroup->setContentLayout(detailsLayout);

        // �����ʾ
        QGroupBox* resultGroup = new QGroupBox(QString("������"));
        QVBoxLayout* resultLayout = new QVBoxLayout(resultGroup);
        m_resultLabel = new QLabel(QString(""));
        m_resultLabel->setWordWrap(true);
        m_resultLabel->setStyleSheet("padding: 10px; background-color: #f0f0f0; border-radius: 5px;");
        resultLayout->addWidget(m_resultLabel);
        mainLayout->addWidget(resultGroup);
        mainLayout->addWidget(detailsGroup);

        // ��ť����
        QHBoxLayout* buttonLayout = new QHBoxLayout();
        QPushButton* saveBtn = new QPushButton(QString("����"));
        QPushButton* cancelBtn = new QPushButton(QString("ȡ��"));

        connect(saveBtn, &QPushButton::clicked, this, &TimeSettingDialog::saveAndClose);
        connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

        buttonLayout->addStretch();
        buttonLayout->addWidget(saveBtn);
        buttonLayout->addWidget(cancelBtn);
        mainLayout->addLayout(buttonLayout);

        // ����ʱ��仯�źţ��Զ����¼���
        //connect(m_needAverageCalCheckBox, &QCheckBox::stateChanged, this, &TimeSettingDialog::calculateWorkTime);
        connect(m_arrivalTimeEdit, &QTimeEdit::timeChanged, this, &TimeSettingDialog::calculateWorkTime);
        connect(m_departureTimeEdit, &QTimeEdit::timeChanged, this, &TimeSettingDialog::calculateWorkTime);
        connect(m_workStartTimeEdit, &QTimeEdit::timeChanged, this, &TimeSettingDialog::calculateWorkTime);
        connect(m_workEndTimeEdit, &QTimeEdit::timeChanged, this, &TimeSettingDialog::calculateWorkTime);
        connect(m_lunchBreakStartEdit, &QTimeEdit::timeChanged, this, &TimeSettingDialog::calculateWorkTime);
        connect(m_lunchBreakEndEdit, &QTimeEdit::timeChanged, this, &TimeSettingDialog::calculateWorkTime);
        connect(m_dinnerBreakStartEdit, &QTimeEdit::timeChanged, this, &TimeSettingDialog::calculateWorkTime);
        connect(m_dinnerBreakEndEdit, &QTimeEdit::timeChanged, this, &TimeSettingDialog::calculateWorkTime);
    }

    void loadRecord()
    {
        QSettings settings;
        QString key = m_date.toString("yyyy-MM-dd");

        m_needAverageCalCheckBox->setChecked(
            settings.value(key + "/needAverageCal", true).toBool());
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

    void saveRecord()
    {
        QSettings settings;
        QString key = m_date.toString("yyyy-MM-dd");

        settings.setValue(key + "/needAverageCal", m_needAverageCalCheckBox->isChecked());
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
    QCheckBox* m_needAverageCalCheckBox;
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

// �Զ��������ؼ���֧���Ҽ��˵�
class CustomCalendarWidget : public QCalendarWidget {
    Q_OBJECT
private:
    QTableView* m_tableView { nullptr };

public:
    CustomCalendarWidget(QWidget* parent = nullptr) :
        QCalendarWidget(parent)
    {
        setContextMenuPolicy(Qt::CustomContextMenu);
        connect(this, &QWidget::customContextMenuRequested, this, &CustomCalendarWidget::showContextMenu);
        setupEventFilters();
    }

    void setupEventFilters()
    {
        m_tableView = this->findChild<QTableView*>();
        if (m_tableView) {
            // ֻ�����Ҽ���˫������д�ķ�������
            m_tableView->installEventFilter(this);
        }
    }

signals:
    void deleteRequested(const QDate& date);

private slots:

    void showContextMenu(const QPoint& pos)
    {
        // ��ȡ���λ�ö�Ӧ������
        QDate clickedDate = dateAt(pos);
        if (!clickedDate.isValid()) {
            return;
        }

        // ���������Ƿ��м�¼
        QSettings settings;
        QString key = clickedDate.toString("yyyy-MM-dd");
        if (!settings.contains(key + "/arrival")) {
            return; // û�м�¼������ʾ�˵�
        }

        // �����Ҽ��˵�
        QMenu contextMenu(this);
        QAction* deleteAction =
            contextMenu.addAction(QString("ɾ�� %1 �ļ�¼").arg(clickedDate.toString("yyyy-MM-dd")));
        deleteAction->setIcon(style()->standardIcon(QStyle::SP_TrashIcon));

        // ��ʾ�˵�������ѡ��
        QAction* selectedAction = contextMenu.exec(mapToGlobal(pos));
        if (selectedAction == deleteAction) {
            emit deleteRequested(clickedDate);
        }
    }

private:
    QDate dateAt(const QPoint& pos)
    {
        // ����һ���򻯵�ʵ�֣���ʵ��ʹ���п�����Ҫ����ȷ�ļ���
        // ʹ��selectedDate��Ϊ����ֵ
        return getDateFromPosition(QPoint(pos.x(), pos.y() - 20));
    }

    QDate getDateFromPosition(const QPoint& pos)
    {
        if (!m_tableView) {
            return QDate();
        }

        QModelIndex index = m_tableView->indexAt(pos);
        if (!index.isValid()) {
            return QDate();
        }

        // ��ȡģ��
        QAbstractItemModel* model = m_tableView->model();
        if (!model) {
            return QDate();
        }

        // ���Բ�ͬ�Ľ�ɫ��ȡ��������
        QVariant dateData;

        // ���� Qt::UserRole
        dateData = model->data(index, Qt::UserRole);
        if (dateData.canConvert<QDate>()) {
            return dateData.toDate();
        }

        // ���� Qt::UserRole + 1
        dateData = model->data(index, Qt::UserRole + 1);
        if (dateData.canConvert<QDate>()) {
            return dateData.toDate();
        }

        // �������ȡ������ʹ�øĽ��ļ��㷽��
        return calculateDateFromRowCol(index.row(), index.column());
    }

    QDate calculateDateFromRowCol(int row, int col)
    {
        // ��ȡ��ǰ��ʾ������
        int year = yearShown();
        int month = monthShown();
        QDate firstDay(year, month, 1);

        // ��ȡ�����ĵ�һ������
        Qt::DayOfWeek startDay = firstDayOfWeek();

        // �����һ���ڱ���е�λ��
        int firstDayColumn;
        if (startDay == Qt::Sunday) {
            firstDayColumn = firstDay.dayOfWeek() % 7; // Sunday = 0
        }
        else {
            firstDayColumn = firstDay.dayOfWeek() - 1; // Monday = 0
        }

        // ���㵱ǰλ�ö�Ӧ������
        int totalCells = (row - 1) * 7 + col - 1;
        int dayNumber = totalCells - firstDayColumn + 1;

        if (dayNumber <= 0) {
            // �ϸ��µ�����
            QDate prevMonth = firstDay.addMonths(-1);
            return QDate(prevMonth.year(), prevMonth.month(), prevMonth.daysInMonth() + dayNumber);
        }
        else if (dayNumber > firstDay.daysInMonth()) {
            // �¸��µ�����
            QDate nextMonth = firstDay.addMonths(1);
            return QDate(nextMonth.year(), nextMonth.month(), dayNumber - firstDay.daysInMonth());
        }
        else {
            // ��ǰ�µ�����
            return QDate(year, month, dayNumber);
        }
    }
};

// ������
class AttendanceMainWindow : public QMainWindow {
    Q_OBJECT

public:
    AttendanceMainWindow(QWidget* parent = nullptr) :
        QMainWindow(parent)
    {
        setWindowTitle(QString("�򿨹���ϵͳ"));
        setMinimumSize(800, 600);

        resize(900, 660);

        setupUI();
        loadAttendanceData();
    }

protected:
    void mousePressEvent(QMouseEvent* event) override
    {
        // �����λ���Ƿ�������������
        if (m_calendar) {
            QPoint calendarPos = m_calendar->mapFromGlobal(event->globalPos());
            QRect calendarRect = m_calendar->rect();

            // �������������⣬����ѡ��״̬
            if (!calendarRect.contains(calendarPos)) {
                // ��ѡ������Ϊ�����������ڣ�����ҳ����ص�ǰҳ�棬������������ʧȥ����
                // m_calendar->setSelectedDate(QDate::currentDate());
                m_calendar->setSelectedDate(QDate::currentDate().addDays(365));
                m_calendar->setCurrentPage(QDate::currentDate().year(), QDate::currentDate().month());
            }
        }

        QMainWindow::mousePressEvent(event);
    }

private slots:

    void onDateClicked(const QDate& date)
    {
        TimeSettingDialog dialog(date, this);
        if (dialog.exec() == QDialog::Accepted) {
            updateCalendarAppearance();
            updateMonthlyStatistics();
        }
    }

    void onMonthChanged()
    {
        updateCalendarAppearance();
        updateMonthlyStatistics();
    }

    void onDeleteRequested(const QDate& date)
    {
        // ȷ��ɾ��
        int ret = QMessageBox::question(
            this,
            QString("ȷ��ɾ��"),
            QString("ȷ��Ҫɾ�� %1 �Ŀ��ڼ�¼��").arg(date.toString("yyyy-MM-dd")),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);

        if (ret == QMessageBox::Yes) {
            deleteAttendanceRecord(date);
        }
    }

private:
    void setupUI()
    {
        QWidget* centralWidget = new QWidget();
        setCentralWidget(centralWidget);

        QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);

        // ��ࣺ����
        QVBoxLayout* leftLayout = new QVBoxLayout();

        QLabel* titleLabel = new QLabel(QString("��������"));
        titleLabel->setAlignment(Qt::AlignCenter);
        titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; padding: 10px;");
        leftLayout->addWidget(titleLabel);

        // ʹ���Զ��������ؼ�
        m_calendar = new CustomCalendarWidget();
        m_calendar->setLocale(QLocale::Chinese);
        m_calendar->setFirstDayOfWeek(Qt::Monday);
        m_calendar->setGridVisible(true);
        leftLayout->addWidget(m_calendar);

        // ���ʹ��˵��
        QLabel* helpLabel =
            new QLabel(QString("ʹ��˵����\n? �������������ÿ���ʱ��\n? �Ҽ�����м�¼�����ڿ�ɾ����¼\n? "
                               "������������������ѡ��״̬"));
        helpLabel->setStyleSheet(
            "color: #666; font-size: 12px; padding: 10px; background-color: #f5f5f5; border-radius: 5px;");
        helpLabel->setWordWrap(true);
        leftLayout->addWidget(helpLabel);

        // �Ҳࣺͳ�ƺ͹���
        QVBoxLayout* rightLayout = new QVBoxLayout();

        // �¶�ͳ��
        QGroupBox* statsGroup = new QGroupBox(QString("�¶�ͳ��"));
        QVBoxLayout* statsLayout = new QVBoxLayout(statsGroup);
        m_statsLabel = new QLabel(QString("��ѡ���·ݲ鿴ͳ��"));
        m_statsLabel->setWordWrap(true);
        m_statsLabel->setStyleSheet("padding: 10px; background-color: #f9f9f9; border-radius: 5px;");
        statsLayout->addWidget(m_statsLabel);
        rightLayout->addWidget(statsGroup);
        rightLayout->addStretch();

        // ʹ�÷ָ���
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

        // �����ź�
        connect(m_calendar, &QCalendarWidget::clicked, this, &AttendanceMainWindow::onDateClicked);
        connect(m_calendar, &QCalendarWidget::currentPageChanged, this, &AttendanceMainWindow::onMonthChanged);
        connect(m_calendar, &CustomCalendarWidget::deleteRequested, this, &AttendanceMainWindow::onDeleteRequested);

        updateCalendarAppearance();
        updateMonthlyStatistics();
    }

    void loadAttendanceData()
    {
        // ����ͨ��QSettings�Զ�����
    }

    void deleteAttendanceRecord(const QDate& date)
    {
        QSettings settings;
        QString key = date.toString("yyyy-MM-dd");

        // ɾ��������ص�������
        QStringList keys = {
            key + "/needAverageCal",
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

        // ���½���
        updateCalendarAppearance();
        updateMonthlyStatistics();

        // ��ʾɾ���ɹ���Ϣ
        QMessageBox::information(
            this,
            QString("ɾ���ɹ�"),
            QString("�ѳɹ�ɾ�� %1 �Ŀ��ڼ�¼").arg(date.toString("yyyy-MM-dd")));
    }

    void updateCalendarAppearance()
    {
        int year = m_calendar->yearShown();
        int month = m_calendar->monthShown();
        QDate startDate(year, month, 1);
        QDate endDate = startDate.addMonths(1).addDays(-1);

        QSettings settings;

        QDate date = startDate;
        while (date <= endDate) {
            QString key = date.toString("yyyy-MM-dd");

            if (settings.contains(key + "/arrival")) {
                // �д򿨼�¼����ʾ��ɫ����
                QTextCharFormat format;
                QColor defaultCol(144, 238, 144); // ǳ��ɫ
                if (settings.contains(key + "/needAverageCal")) {
                    if (!settings.value(key + "/needAverageCal").toBool()) {
                        defaultCol = QColor("#acfdea"); 
                    }
                }
                format.setBackground(defaultCol);
                m_calendar->setDateTextFormat(date, format);
            }
            else {
                // �����ʽ
                m_calendar->setDateTextFormat(date, QTextCharFormat());
            }
            date = date.addDays(1);
        }
    }

    void updateMonthlyStatistics()
    {
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

                // ���ؼ�¼������
                AttendanceRecord record;
                record.needAverageCal = settings.value(key + "/needAverageCal").toBool();
                record.arrivalTime = QTime::fromString(settings.value(key + "/arrival").toString(), "hh:mm");
                record.departureTime = QTime::fromString(settings.value(key + "/departure").toString(), "hh:mm");
                record.workStartTime =
                    QTime::fromString(settings.value(key + "/workStart", "09:00").toString(), "hh:mm");
                record.workEndTime = QTime::fromString(settings.value(key + "/workEnd", "18:00").toString(), "hh:mm");
                record.lunchBreakStart =
                    QTime::fromString(settings.value(key + "/lunchStart", "12:30").toString(), "hh:mm");
                record.lunchBreakEnd =
                    QTime::fromString(settings.value(key + "/lunchEnd", "13:30").toString(), "hh:mm");
                record.dinnerBreakStart =
                    QTime::fromString(settings.value(key + "/dinnerStart", "18:00").toString(), "hh:mm");
                record.dinnerBreakEnd =
                    QTime::fromString(settings.value(key + "/dinnerEnd", "18:30").toString(), "hh:mm");


                if (!record.needAverageCal)workDays--;
                // ���㹤��ʱ������
                WorkTimeResult result = WorkTimeCalculator::calculateWorkTimeResult(record);

                if (result.overtimeMinutes > 0) {
                    totalOvertimeMinutes += result.overtimeMinutes;
                }
                totalLateMinutes += result.lateMinutes;
                totalEarlyLeaveMinutes += result.earlyLeaveMinutes;
            }
            date = date.addDays(1);
        }

        QString stats = QString("ͳ���·�: %1��%2��\n").arg(year).arg(month);
        stats += QString("��������: %1��\n").arg(workDays);
        stats += QString("总加班时间: %1小时%2分钟\n")
                     .arg(totalOvertimeMinutes / 60)
                     .arg(totalOvertimeMinutes % 60);
        stats += QString("总迟到时间: %1小时%2分钟\n")
                     .arg(totalLateMinutes / 60)
                     .arg(totalLateMinutes % 60);
        stats += QString("总早退时间: %1小时%2分钟\n")
                     .arg(totalEarlyLeaveMinutes / 60)
                     .arg(totalEarlyLeaveMinutes % 60);
        if (workDays) {
            stats += QString("平均加班时间: %1小时")
                .arg(totalOvertimeMinutes / (60.0*workDays));
        }


        m_statsLabel->setText(stats);
    }

private:
    CustomCalendarWidget* m_calendar;
    QLabel* m_statsLabel;
};

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    // Qt5�ı�������
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
#endif
    // ��main.cpp�����Ӧ��ͼ��

    app.setWindowIcon(QIcon(":icon.ico"));

    // ����Ӧ�ó�����Ϣ
    app.setApplicationName("AttendanceApp");
    app.setOrganizationName("MyCompany");

    // ����Ĭ������
    QFont font = app.font();
    font.setFamily("Microsoft YaHei");
    font.setPointSize(9);
    app.setFont(font);

    AttendanceMainWindow window;
    window.show();

    return app.exec();
}

#include "main.moc"
