#pragma once

#include "SoapClient.h"

#include <QObject>

// SOAP client for the UPnP AVTransport service (casting control).
class AvTransportClient : public QObject
{
    Q_OBJECT

public:
    explicit AvTransportClient(QObject *parent = nullptr);

    void setControlUrl(const QUrl &url);

    void setUri(const QUrl &mediaUrl, const QString &title,
                const QString &mimeType, qint64 sizeBytes, bool live = false);
    void play();
    void pause();
    void stop();
    void seek(qint64 positionMs);
    void getPositionInfo();
    void getTransportInfo();

signals:
    void uriSet();
    void positionInfoReceived(qint64 positionMs, qint64 durationMs);
    void transportStateChanged(const QString &state); // PLAYING / PAUSED_PLAYBACK / STOPPED / ...
    void actionFailed(const QString &action, const QString &error);

private:
    static QString formatTime(qint64 ms);
    static qint64 parseTime(const QString &hhmmss);

    SoapClient m_soap;
    QUrl m_controlUrl;
};