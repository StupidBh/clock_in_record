#include "Application/AttendanceMainWindow.h"
#include "Application/Theme.h"
#include <QApplication>
#include <QFont>
#include <QIcon>
#include <QLocalServer>
#include <QLocalSocket>
#include <QStyleFactory>
#include <QStyleHints>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

    app.setWindowIcon(QIcon(":/Icons/logo.ico"));
    app.setApplicationName("AttendanceApp");
    app.setOrganizationName("MyCompany");

    QFont font = app.font();
    font.setFamily("Microsoft YaHei");
    font.setPointSize(9);
    app.setFont(font);

    AttendanceTheme::apply(app, app.styleHints()->colorScheme());
    QObject::connect(
        app.styleHints(), &QStyleHints::colorSchemeChanged, &app, [&app](const Qt::ColorScheme colorScheme) {
            AttendanceTheme::apply(app, colorScheme);
        });

    const QString serverName = QStringLiteral("AttendanceApp-SingleInstance");

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

    QObject::connect(&localServer, &QLocalServer::newConnection, &window, [&window, &localServer]() {
        while (auto* client = localServer.nextPendingConnection()) {
            client->disconnectFromServer();
            client->deleteLater();
        }
        window.raiseAndActivate();
    });

    window.show();

    return app.exec();
}
