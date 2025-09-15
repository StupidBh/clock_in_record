#include "AttendanceMainWindow.h"
#include <QApplication>
#include <QFont>
#include <QIcon>
#include <QTextCodec>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);


    // ����Ӧ�ó���ͼ�꣨����еĻ���
     app.setWindowIcon(QIcon(":/Icons/logo.ico"));
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
