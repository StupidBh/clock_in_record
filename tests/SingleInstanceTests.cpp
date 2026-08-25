#include "Application/SingleInstance.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QLocalServer>
#include <QLocalSocket>
#include <QThread>
#include <QUuid>

#include <chrono>
#include <future>
#include <iostream>

using namespace Qt::StringLiterals;

namespace {
    int failures = 0;

    void expectTrue(const char* name, const bool value)
    {
        if (value) {
            return;
        }

        std::cerr << name << ": expected true\n";
        ++failures;
    }

    template<typename Result>
    bool processEventsUntilReady(std::future<Result>& future, const int timeoutMilliseconds)
    {
        QElapsedTimer timer;
        timer.start();
        while (future.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready &&
               timer.elapsed() < timeoutMilliseconds) {
            QCoreApplication::processEvents();
            QThread::msleep(1);
        }
        QCoreApplication::processEvents();
        return future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready;
    }

    QString uniqueServerName(const QString& suffix)
    {
        return u"%1.%2.%3"_s.arg(AttendanceSingleInstance::serverName(),
                                 suffix,
                                 QUuid::createUuid().toString(QUuid::WithoutBraces));
    }

    void testActivationHandshake()
    {
        QLocalServer server;
        const QString name = uniqueServerName(u"handshake"_s);
        expectTrue("single instance server listens", AttendanceSingleInstance::listen(server, name));
        expectTrue("single instance server is user restricted",
                   server.socketOptions() == QLocalServer::UserAccessOption);

        bool activated = false;
        QObject::connect(&server, &QLocalServer::newConnection, &server, [&server, &activated]() {
            AttendanceSingleInstance::acceptPendingConnections(server, [&activated]() { activated = true; });
        });

        auto notification = std::async(std::launch::async, [name]() {
            return AttendanceSingleInstance::notifyRunningInstance(name, 2000);
        });
        expectTrue("activation notification completes", processEventsUntilReady(notification, 3000));
        expectTrue("activation notification is acknowledged", notification.get());
        expectTrue("activation callback runs", activated);
    }

    void testUnrecognizedServerIsRejected()
    {
        QLocalServer unrelatedServer;
        const QString name = uniqueServerName(u"unrecognized"_s);
        expectTrue("unrecognized server listens", unrelatedServer.listen(name));
        QObject::connect(&unrelatedServer, &QLocalServer::newConnection, &unrelatedServer, [&unrelatedServer]() {
            while (QLocalSocket* const client = unrelatedServer.nextPendingConnection()) {
                client->write(QByteArrayLiteral("not-attendance-app\n"));
                client->disconnectFromServer();
                client->deleteLater();
            }
        });

        auto notification = std::async(std::launch::async, [name]() {
            return AttendanceSingleInstance::notifyRunningInstance(name, 2000);
        });
        expectTrue("unrecognized server response completes", processEventsUntilReady(notification, 3000));
        expectTrue("unrecognized server is rejected", !notification.get());
    }
} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication application(argc, argv);
    QCoreApplication::setOrganizationName(u"AttendanceAppTests"_s);
    QCoreApplication::setApplicationName(u"SingleInstanceTests"_s);

    expectTrue("server name is scoped", AttendanceSingleInstance::serverName().contains(u"AttendanceAppTests"_s));
    testActivationHandshake();
    testUnrecognizedServerIsRejected();
    return failures == 0 ? 0 : 1;
}
