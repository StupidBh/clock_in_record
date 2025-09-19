#include "WorkTimeCalculator.h"

WorkTimeResult WorkTimeCalculator::calculateWorkTimeResult(const AttendanceRecord& record) {
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
    if (isTimeRangeOverlap(record.arrivalTime, record.departureTime,
        record.lunchBreakStart, record.lunchBreakEnd)) {
        QTime lunchStart = maxTime(record.arrivalTime, record.lunchBreakStart);
        QTime lunchEnd = minTime(record.departureTime, record.lunchBreakEnd);
        if (lunchStart < lunchEnd) {
            result.totalBreakMinutes += lunchStart.secsTo(lunchEnd) / 60;
        }
    }

    // ���ʱ��
    if (isTimeRangeOverlap(record.arrivalTime, record.departureTime,
        record.dinnerBreakStart, record.dinnerBreakEnd)) {
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
    if (record.lunchBreakStart >= record.workStartTime &&
        record.lunchBreakStart < record.workEndTime &&
        record.lunchBreakStart < record.lunchBreakEnd) {
        standardBreakMinutes += record.lunchBreakStart.secsTo(record.lunchBreakEnd) / 60;
    }

    // ��׼���ʱ�䣨����ڹ���ʱ���ڣ�
    if (record.dinnerBreakStart >= record.workStartTime &&
        record.dinnerBreakStart < record.workEndTime) {
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

bool WorkTimeCalculator::isTimeRangeOverlap(const QTime& start1, const QTime& end1,
    const QTime& start2, const QTime& end2) {
    return start1 < end2 && start2 < end1;
}

QTime WorkTimeCalculator::maxTime(const QTime& time1, const QTime& time2) {
    return time1 > time2 ? time1 : time2;
}

QTime WorkTimeCalculator::minTime(const QTime& time1, const QTime& time2) {
    return time1 < time2 ? time1 : time2;
}