#include "AttendanceMainWindow.h"
#include <QApplication>
#include <QFont>
#include <QIcon>
#include <QTextCodec>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);


    // 设置应用程序图标（如果有的话）
     app.setWindowIcon(QIcon(":/Icons/logo.ico"));
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
