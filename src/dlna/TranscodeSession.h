#pragma once

#include <QObject>
#include <QProcess>
#include <QTcpServer>
#include <QTcpSocket>

// Runs ffmpeg to transcode a local file into a live H.264+AAC MPEG-TS stream
// over a loopback TCP connection, for renderers that cannot play the original
// format directly. TCP flow control bounds memory when the TV reads slowly.
class TranscodeSession : public QObject
{
    Q_OBJECT

public:
    explicit TranscodeSession(QObject *parent = nullptr);
    ~TranscodeSession() override;

    bool start(const QString &filePath);
    void stop();
    bool isRunning() const;

    // Read side of the MPEG-TS stream (loopback socket fed by ffmpeg).
    QIODevice *stream();

    static QString ffmpegPath();

signals:
    void streamFinished();
    void errorOccurred(const QString &message);

private:
    static QStringList encoderArgs();

    QProcess m_process;
    QTcpServer m_outputServer;
    QTcpSocket *m_socket = nullptr;
    bool m_stopping = false;
};