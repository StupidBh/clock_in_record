#include "Application/AttendanceMainWindow.h"
#include "Application/SingleInstance.h"
#include "Application/Theme.h"

#include <QApplication>
#include <QFont>
#include <QIcon>
#include <QLocalServer>
#include <QStyleFactory>
#include <QStyleHints>

using namespace Qt::StringLiterals;

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

    const QString serverName = AttendanceSingleInstance::serverName();

    if (AttendanceSingleInstance::notifyRunningInstance(serverName)) {
        return 0;
    }

    QLocalServer localServer;
    if (!AttendanceSingleInstance::listen(localServer, serverName)) {
        // Another instance can win the race between the initial connection attempt and listen().
        if (AttendanceSingleInstance::notifyRunningInstance(serverName)) {
            return 0;
        }

        QLocalServer::removeServer(serverName);
        if (!AttendanceSingleInstance::listen(localServer, serverName)) {
            qFatal("Failed to start single-instance server");
        }
    }

    AttendanceMainWindow window;

    QObject::connect(&localServer, &QLocalServer::newConnection, &window, [&window, &localServer]() {
        AttendanceSingleInstance::acceptPendingConnections(localServer, [&window]() { window.raiseAndActivate(); });
    });

    window.show();

    return app.exec();
}
