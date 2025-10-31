#include "AttendanceMainWindow.h"
#include <QApplication>
#include <QFont>
#include <QIcon>
#include <QTextCodec>
#include <qsslsocket.h>

#include<Windows.h>

void messageOutput(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    switch (type) {
    case QtDebugMsg:
        //msg.toStdString();
        break;
    case QtWarningMsg:
        //mylogger->warn(msg.toStdString());
        break;
    case QtCriticalMsg:
        //mylogger->critical(msg.toStdString());
        break;
    case QtFatalMsg:
        //mylogger->error(msg.toStdString());
        break;
    case QtInfoMsg:
        //mylogger->info(msg.toStdString());
        break;
    }

    // 输出到控制台
    QTextStream console(stdout);
    console << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz")
        << " " << msg << Qt::endl;
}


int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    AllocConsole();
    FILE* fp;
    freopen_s(&fp, "CONIN$", "r", stdin);
    freopen_s(&fp, "CONOUT$", "w", stdout);

    // 安装日志处理器
    qInstallMessageHandler(messageOutput);

    qDebug() << "SSL supported:" << QSslSocket::supportsSsl();
    qDebug() << "OpenSSL version:" << QSslSocket::sslLibraryVersionString();

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
