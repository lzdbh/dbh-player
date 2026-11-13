#pragma once

#include <QObject>
#include <QProcess>

// Runs ffmpeg to transcode a local file into a live H.264+AAC MPEG-TS stream
// on stdout, for renderers that cannot play the original format directly.
class TranscodeSession : public QObject
{
    Q_OBJECT

public:
    explicit TranscodeSession(QObject *parent = nullptr);
    ~TranscodeSession() override;

    bool start(const QString &filePath);
    void stop();
    bool isRunning() const;

    // Read side of the MPEG-TS stream (the QProcess stdout channel).
    QIODevice *stream();

    static QString ffmpegPath();

signals:
    void streamFinished();
    void errorOccurred(const QString &message);

private:
    static QStringList encoderArgs();

    QProcess m_process;
    bool m_stopping = false;
};