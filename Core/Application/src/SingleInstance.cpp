#include "Application/SingleInstance.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QElapsedTimer>
#include <QLocalServer>
#include <QLocalSocket>
#include <QStandardPaths>
#include <QTimer>

#include <memory>
#include <utility>

using namespace Qt::StringLiterals;

namespace {
    constexpr int ClientRequestTimeoutMilliseconds = 1000;
    const QByteArray ActivationRequest = QByteArrayLiteral("AttendanceApp/1 activate\n");
    const QByteArray ActivationAcknowledgement = QByteArrayLiteral("AttendanceApp/1 activated\n");

    void configureClient(QLocalSocket& client, std::function<void()> activate)
    {
        auto request = std::make_shared<QByteArray>();
        auto consumeRequest = [&client, request, activate = std::move(activate)]() {
            request->append(client.readAll());
            if (!ActivationRequest.startsWith(*request)) {
                client.abort();
                return;
            }
            if (*request != ActivationRequest) {
                return;
            }

            client.write(ActivationAcknowledgement);
            client.flush();
            activate();
            client.disconnectFromServer();
        };

        QObject::connect(&client, &QLocalSocket::readyRead, &client, consumeRequest);
        QObject::connect(&client, &QLocalSocket::disconnected, &client, &QObject::deleteLater);
        QTimer::singleShot(ClientRequestTimeoutMilliseconds, &client, [&client]() {
            if (client.state() != QLocalSocket::UnconnectedState) {
                client.abort();
            }
        });

        if (client.bytesAvailable() > 0) {
            consumeRequest();
        }
    }
} // namespace

namespace AttendanceSingleInstance {
    QString serverName()
    {
        const QByteArray userScope = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation).toUtf8();
        const QByteArray scopeHash = QCryptographicHash::hash(userScope, QCryptographicHash::Sha256).toHex().left(16);
        return u"%1.%2.SingleInstance.%3"_s.arg(QCoreApplication::organizationName(),
                                                QCoreApplication::applicationName(),
                                                QString::fromLatin1(scopeHash));
    }

    bool listen(QLocalServer& server, const QString& name)
    {
        server.setSocketOptions(QLocalServer::UserAccessOption);
        return server.listen(name);
    }

    bool notifyRunningInstance(const QString& name, const int timeoutMilliseconds)
    {
        QLocalSocket socket;
        socket.connectToServer(name);
        if (!socket.waitForConnected(timeoutMilliseconds) ||
            socket.write(ActivationRequest) != ActivationRequest.size() ||
            !socket.waitForBytesWritten(timeoutMilliseconds)) {
            return false;
        }

        QByteArray acknowledgement;
        QElapsedTimer timer;
        timer.start();
        while (ActivationAcknowledgement.startsWith(acknowledgement) &&
               acknowledgement.size() < ActivationAcknowledgement.size()) {
            const int remainingMilliseconds = timeoutMilliseconds - static_cast<int>(timer.elapsed());
            if (remainingMilliseconds <= 0 || !socket.waitForReadyRead(remainingMilliseconds)) {
                return false;
            }
            acknowledgement += socket.readAll();
        }
        return acknowledgement == ActivationAcknowledgement;
    }

    void acceptPendingConnections(QLocalServer& server, std::function<void()> activate)
    {
        while (QLocalSocket* const client = server.nextPendingConnection()) {
            configureClient(*client, activate);
        }
    }
} // namespace AttendanceSingleInstance
