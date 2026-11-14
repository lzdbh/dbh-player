#pragma once

#include <QObject>
#include <QUrl>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

#include <mpv/client.h>

// libmpv-based playback backend. The mpv core lives on the main (GUI) thread
// and its event queue is drained by a periodic timer — independent of mpv's
// wakeup callback, which avoids a deadlock where mpv blocks waiting for the
// client to drain events during audio initialization. Video is rendered
// through the mpv render API by MpvVideoItem (OpenGL FBO, render thread).
class MpvPlayer : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(bool playing READ playing NOTIFY playingChanged)
    Q_PROPERTY(qint64 position READ position NOTIFY positionChanged)
    Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(float volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(bool muted READ muted WRITE setMuted NOTIFY mutedChanged)
    Q_PROPERTY(float playbackRate READ playbackRate WRITE setPlaybackRate NOTIFY playbackRateChanged)
    Q_PROPERTY(bool hasVideo READ hasVideo NOTIFY hasVideoChanged)

public:
    explicit MpvPlayer(QObject *parent = nullptr);
    ~MpvPlayer() override;

    mpv_handle *handle() const { return m_mpv; }

    QUrl source() const;
    void setSource(const QUrl &url);
    bool playing() const;
    qint64 position() const;
    qint64 duration() const;
    float volume() const;
    void setVolume(float volume);
    bool muted() const;
    void setMuted(bool muted);
    float playbackRate() const;
    void setPlaybackRate(float rate);
    bool hasVideo() const;

    Q_INVOKABLE void open(const QUrl &url);
    Q_INVOKABLE void play();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void togglePlay();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void seek(qint64 positionMs);
    Q_INVOKABLE void setRenderReady();

signals:
    void sourceChanged();
    void playingChanged();
    void positionChanged();
    void durationChanged();
    void volumeChanged();
    void mutedChanged();
    void playbackRateChanged();
    void hasVideoChanged();
    void endOfMedia();
    void errorOccurred(const QString &message);

private slots:
    void pollEvents();

private:
    void command(const QStringList &args);
    void setMpvProperty(const QString &name, const QVariant &value);
    void loadPending();

    mpv_handle *m_mpv = nullptr;
    QTimer m_eventTimer;
    bool m_renderReady = false;

    QUrl m_source;
    bool m_playing = false;
    qint64 m_position = 0;
    qint64 m_lastLogPos = -1000;
    qint64 m_duration = 0;
    float m_volume = 1.0f;
    bool m_muted = false;
    float m_rate = 1.0f;
    bool m_hasVideo = false;
};