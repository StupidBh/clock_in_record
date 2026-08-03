#include "AttendanceMainWindow.h"
#include <QApplication>
#include <QFont>
#include <QIcon>
#include <QLocalServer>
#include <QLocalSocket>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    app.setWindowIcon(QIcon(":/Icons/logo.ico"));
    app.setApplicationName("AttendanceApp");
    app.setOrganizationName("MyCompany");

    QFont font = app.font();
    font.setFamily("Microsoft YaHei");
    font.setPointSize(9);
    app.setFont(font);

    const QString serverName = "AttendanceApp-SingleInstance";

    // Step 1: try to connect to an existing instance
    {
        QLocalSocket socket;
        socket.connectToServer(serverName);
        if (socket.waitForConnected(500)) {
            socket.write("activate");
            socket.waitForBytesWritten(500);
            return 0;
        }
    }

    // Step 2: no living instance — clean up any stale name and become the server
    QLocalServer::removeServer(serverName);
    QLocalServer localServer;
    if (!localServer.listen(serverName)) {
        qFatal("Failed to start single-instance server");
    }

    AttendanceMainWindow window;

    QObject::connect(&localServer, &QLocalServer::newConnection, [&window, &localServer]() {
        auto* client = localServer.nextPendingConnection();
        client->deleteLater();
        window.raiseAndActivate();
    });

    window.show();

    return app.exec();
}
