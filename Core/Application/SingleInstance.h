#pragma once

#include <QString>

#include <functional>

class QLocalServer;

namespace AttendanceSingleInstance {
    [[nodiscard]] QString serverName();
    [[nodiscard]] bool listen(QLocalServer& server, const QString& name);
    [[nodiscard]] bool notifyRunningInstance(const QString& name, int timeoutMilliseconds = 500);
    void acceptPendingConnections(QLocalServer& server, std::function<void()> activate);
} // namespace AttendanceSingleInstance
