#pragma once
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVersionNumber>
#include <QApplication>
#include <QTimer>

class GitHubUpdater : public QObject {
    Q_OBJECT
public:
    explicit GitHubUpdater(
        const QString& repoOwner,
        const QString& repoName,
        const QString& currentVersion,
        QObject* parent = nullptr);

    void checkForUpdates();
    void setDownloadDir(const QString& dir); // 可选：自定义下载目录

signals:
    void updateAvailable(const QString& version, const QString& changelog);
    void updateDownloaded(const QString& filePath);
    void noUpdateAvailable();
    void errorOccurred(const QString& message);

private slots:
    void onCheckReplyFinished(QNetworkReply* reply);
    void onDownloadFinished(QNetworkReply* reply);

private:
    QString findDownloadUrl(const QJsonObject& releaseObj);
    void startDownload(const QUrl& url);

    QNetworkAccessManager m_netManager;
    QString m_repoOwner;
    QString m_repoName;
    QString m_currentVersion;
    QString m_downloadDir;
    QString m_downloadFileName;
};
