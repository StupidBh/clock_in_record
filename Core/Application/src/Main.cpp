#include "Application/AttendanceMainWindow.h"
#include "Application/Theme.h"

#include <QApplication>
#include <QByteArray>
#include <QFont>
#include <QIcon>
#include <QLocalServer>
#include <QLocalSocket>
#include <QStyleFactory>
#include <QStyleHints>

using namespace Qt::StringLiterals;

namespace {
    constexpr int ConnectionTimeoutMilliseconds = 500;

    [[nodiscard]] bool notifyRunningInstance(const QString& serverName)
    {
        QLocalSocket socket;
        socket.connectToServer(serverName);
        if (!socket.waitForConnected(ConnectionTimeoutMilliseconds)) {
            return false;
        }

        socket.write(QByteArrayLiteral("activate"));
        socket.waitForBytesWritten(ConnectionTimeoutMilliseconds);
        return true;
    }
} // namespace

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    app.setStyle(QStyleFactory::create(u"Fusion"_s));

    app.setWindowIcon(QIcon(u":/Icons/logo.ico"_s));
    app.setApplicationName(u"AttendanceApp"_s);
    app.setOrganizationName(u"MyCompany"_s);

    QFont font = app.font();
    font.setFamily(u"Microsoft YaHei"_s);
    font.setPointSize(9);
    app.setFont(font);

    AttendanceTheme::apply(app, app.styleHints()->colorScheme());
    QObject::connect(app.styleHints(),
                     &QStyleHints::colorSchemeChanged,
                     &app,
                     [&app](const Qt::ColorScheme colorScheme) { AttendanceTheme::apply(app, colorScheme); });

    const QString serverName = u"AttendanceApp-SingleInstance"_s;

    if (notifyRunningInstance(serverName)) {
        return 0;
    }

    QLocalServer localServer;
    if (!localServer.listen(serverName)) {
        // Another instance can win the race between the initial connection attempt and listen().
        if (notifyRunningInstance(serverName)) {
            return 0;
        }

        QLocalServer::removeServer(serverName);
        if (!localServer.listen(serverName)) {
            qFatal("Failed to start single-instance server");
        }
    }

    AttendanceMainWindow window;

    QObject::connect(&localServer, &QLocalServer::newConnection, &window, [&window, &localServer]() {
        while (auto* const client = localServer.nextPendingConnection()) {
            client->disconnectFromServer();
            client->deleteLater();
        }
        window.raiseAndActivate();
    });

    window.show();

    return app.exec();
}
