#pragma once

#include <QObject>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

// External SRT subtitle track: parses cues and serves the text for a given
// playback position. Encoding is auto-detected (UTF-8/UTF-16/GB18030).
class SubtitleTrack : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool loaded READ loaded NOTIFY loadedChanged)
    Q_PROPERTY(QString fileName READ fileName NOTIFY loadedChanged)
    Q_PROPERTY(int cueCount READ cueCount NOTIFY loadedChanged)

public:
    explicit SubtitleTrack(QObject *parent = nullptr);

    bool loaded() const;
    QString fileName() const;
    int cueCount() const;

    Q_INVOKABLE bool load(const QUrl &fileUrl);
    // Looks for <video basename>.srt next to the video; loads it if present.
    Q_INVOKABLE bool autoLoadFor(const QUrl &videoUrl);
    Q_INVOKABLE void unload();
    Q_INVOKABLE QString textAt(qint64 positionMs) const;

signals:
    void loadedChanged();

private:
    struct Cue
    {
        qint64 startMs;
        qint64 endMs;
        QString text;
    };

    bool parse(const QByteArray &data);

    QList<Cue> m_cues;
    QString m_fileName;
};