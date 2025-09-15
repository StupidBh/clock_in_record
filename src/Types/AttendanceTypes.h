#ifndef ATTENDANCETYPES_H
#define ATTENDANCETYPES_H

#include <QTime>

// �򿨼�¼�ṹ��
struct AttendanceRecord {
    bool needAverageCal;    // �Ƿ����ƽ���Ӱ����
    QTime arrivalTime;      // ���﹫˾ʱ��
    QTime departureTime;    // �뿪��˾ʱ��
    QTime workStartTime;    // ��׼�ϰ�ʱ��
    QTime workEndTime;      // ��׼�°�ʱ��
    QTime lunchBreakStart;  // ��Ϳ�ʼʱ��
    QTime lunchBreakEnd;    // ��ͽ���ʱ��
    QTime dinnerBreakStart; // ��Ϳ�ʼʱ��
    QTime dinnerBreakEnd;   // ��ͽ���ʱ��

    AttendanceRecord() {
        needAverageCal = true;
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
struct WorkTimeResult {
    int actualWorkMinutes = 0;      // ʵ�ʹ���ʱ�䣨���ӣ�
    int standardWorkMinutes = 0;    // ��׼����ʱ�䣨���ӣ�
    int lateMinutes = 0;           // �ٵ�ʱ�䣨���ӣ�
    int lateSize = 0;
    int earlyLeaveMinutes = 0;     // ����ʱ�䣨���ӣ�
    int earlyLeaveSize = 0;
    int overtimeMinutes = 0;       // �Ӱ�ʱ�䣨���ӣ�
    int totalBreakMinutes = 0;     // ����Ϣʱ�䣨���ӣ�
};

#endif // ATTENDANCETYPES_H