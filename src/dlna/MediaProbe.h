#pragma once

#include <QObject>
#include <QUrl>

// Probes a media file with ffprobe and decides whether a DLNA renderer can
// be expected to play it directly (MP4/MOV container + H.264 + AAC/MP3).
class MediaProbe : public QObject
{
    Q_OBJECT

public:
    struct Result
    {
        bool valid = false;
        bool directPushCompatible = false;
        QString videoCodec;
        QString audioCodec;
        QString formatName;
        qint64 durationMs = 0;
        QString error;
    };

    explicit MediaProbe(QObject *parent = nullptr);

    void probe(const QString &filePath);
    static QString ffprobePath();

signals:
    void probed(const MediaProbe::Result &result);
};