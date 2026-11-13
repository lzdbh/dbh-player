#pragma once

#include <QIODevice>
#include <QObject>
#include <QTcpServer>
#include <QUrl>

// Serves the currently cast media over HTTP. File mode supports Range so the
// TV can seek; live mode streams a transcode pipe (no length, no seeking).
// Listens on all interfaces; mediaUrl() returns the LAN URL.
class MediaHttpServer : public QTcpServer
{
    Q_OBJECT

public:
    explicit MediaHttpServer(QObject *parent = nullptr);

    bool start(const QString &filePath);
    bool startLive(QIODevice *stream, const QString &mimeType);
    void stop();

    QUrl mediaUrl() const;
    QString filePath() const;
    QString mimeType() const;
    qint64 fileSize() const;
    bool isLive() const;

    static QString mimeTypeForFile(const QString &filePath);

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private:
    QString m_filePath;
    QString m_mimeType;
    qint64 m_fileSize = 0;
    QIODevice *m_liveStream = nullptr;
    bool m_liveBusy = false;
};