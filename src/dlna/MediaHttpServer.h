#pragma once

#include <QObject>
#include <QTcpServer>
#include <QUrl>

// Serves the currently cast local file over HTTP with Range support so the
// TV can seek. Listens on all interfaces; mediaUrl() returns the LAN URL.
class MediaHttpServer : public QTcpServer
{
    Q_OBJECT

public:
    explicit MediaHttpServer(QObject *parent = nullptr);

    bool start(const QString &filePath);
    void stop();

    QUrl mediaUrl() const;
    QString filePath() const;
    QString mimeType() const;
    qint64 fileSize() const;

    static QString mimeTypeForFile(const QString &filePath);

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private:
    QString m_filePath;
    QString m_mimeType;
    qint64 m_fileSize = 0;
};