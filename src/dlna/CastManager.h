#pragma once

#include "AvTransportClient.h"
#include "MediaHttpServer.h"
#include "MediaProbe.h"
#include "RenderingControlClient.h"
#include "TranscodeSession.h"

#include <QObject>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

// Orchestrates a cast session: local HTTP server -> SetAVTransportURI -> Play,
// then polls the renderer for position/state so QML can show remote controls.
class CastManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool active READ active NOTIFY activeChanged)
    Q_PROPERTY(QString deviceName READ deviceName NOTIFY deviceNameChanged)
    Q_PROPERTY(bool playing READ playing NOTIFY playingChanged)
    Q_PROPERTY(qint64 position READ position NOTIFY positionChanged)
    Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(int volume READ volume NOTIFY volumeChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(bool transcoding READ transcoding NOTIFY transcodingChanged)

public:
    explicit CastManager(QObject *parent = nullptr);

    bool active() const;
    QString deviceName() const;
    bool playing() const;
    qint64 position() const;
    qint64 duration() const;
    int volume() const;
    QString status() const;
    bool transcoding() const;

    Q_INVOKABLE void cast(const QUrl &avTransportUrl, const QUrl &renderingControlUrl,
                          const QString &deviceName, const QUrl &localFile);
    Q_INVOKABLE void play();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void togglePlay();
    Q_INVOKABLE void seek(qint64 positionMs);
    Q_INVOKABLE void setVolume(int volume);
    Q_INVOKABLE void disconnect();

signals:
    void activeChanged();
    void deviceNameChanged();
    void playingChanged();
    void positionChanged();
    void durationChanged();
    void volumeChanged();
    void statusChanged();
    void transcodingChanged();
    void castStarted();
    void castFailed(const QString &message);

private:
    void setActive(bool active);
    void setPlaying(bool playing);
    void setStatus(const QString &status);
    void setTranscoding(bool transcoding);
    void fail(const QString &message);
    void beginPush(const QUrl &mediaUrl, const QString &title,
                   const QString &mimeType, qint64 sizeBytes, bool live);

    MediaHttpServer m_server;
    AvTransportClient m_avTransport;
    RenderingControlClient m_renderingControl;
    MediaProbe m_probe;
    TranscodeSession m_transcode;
    QTimer m_pollTimer;

    bool m_active = false;
    bool m_playing = false;
    bool m_transcoding = false;
    QString m_deviceName;
    QString m_status;
    qint64 m_position = 0;
    qint64 m_duration = 0;
    int m_volume = 50;

    // Pending cast request, kept while ffprobe runs.
    QString m_pendingPath;
    QUrl m_pendingAvt;
    QUrl m_pendingRc;
};