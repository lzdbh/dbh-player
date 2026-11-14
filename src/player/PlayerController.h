#pragma once

#include <QAudioOutput>
#include <QMediaPlayer>
#include <QObject>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

// Wraps QMediaPlayer + QAudioOutput and exposes a clean playback API to QML.
// The media player is created lazily (on first open) to avoid a multimedia
// backend initialization race at startup.
class PlayerController : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QMediaPlayer *mediaPlayer READ mediaPlayer NOTIFY mediaPlayerChanged)
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(bool playing READ playing NOTIFY playingChanged)
    Q_PROPERTY(qint64 position READ position NOTIFY positionChanged)
    Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(float volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(bool muted READ muted WRITE setMuted NOTIFY mutedChanged)
    Q_PROPERTY(float playbackRate READ playbackRate WRITE setPlaybackRate NOTIFY playbackRateChanged)
    Q_PROPERTY(bool hasVideo READ hasVideo NOTIFY hasVideoChanged)

public:
    explicit PlayerController(QObject *parent = nullptr);

    QMediaPlayer *mediaPlayer() const;
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

signals:
    void mediaPlayerChanged();
    void sourceChanged();
    void playingChanged();
    void positionChanged();
    void durationChanged();
    void volumeChanged();
    void mutedChanged();
    void endOfMedia();
    void playbackRateChanged();
    void hasVideoChanged();
    void errorOccurred(const QString &message);
    // Emitted with a temp SRT extracted from an embedded subtitle track, so
    // the UI can render it with its own (transparent, outlined) subtitle style.
    void subtitleFileReady(const QUrl &file);

private:
    void ensurePlayer();
    void extractEmbeddedSubtitle();

    QMediaPlayer *m_player = nullptr;
    QAudioOutput *m_audio = nullptr;
    float m_volume = 1.0f;
    float m_rate = 1.0f;
    bool m_muted = false;
    bool m_subtitleExtractDone = false;
};