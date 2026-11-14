#include "TranscodeSession.h"

#include <QFile>
#include <QStandardPaths>

namespace {
QString findTool(const QString &name)
{
    const QString found = QStandardPaths::findExecutable(name);
    if (!found.isEmpty())
        return found;
    const QStringList extra = {
        QStringLiteral("/usr/local/bin/%1").arg(name),
        QStringLiteral("/opt/homebrew/bin/%1").arg(name),
    };
    for (const QString &path : extra) {
        if (QFile::exists(path))
            return path;
    }
    return {};
}

bool hasEncoder(const QString &ffmpeg, const QString &encoder)
{
    QProcess probe;
    probe.start(ffmpeg, {QStringLiteral("-hide_banner"), QStringLiteral("-encoders")});
    if (!probe.waitForFinished(5000))
        return false;
    return probe.readAllStandardOutput().contains(encoder.toUtf8());
}
} // namespace

TranscodeSession::TranscodeSession(QObject *parent)
    : QObject(parent)
{
    m_process.setProcessChannelMode(QProcess::SeparateChannels);
    connect(&m_process, &QProcess::finished, this, [this](int exitCode) {
        if (m_stopping)
            return;
        if (exitCode == 0) {
            emit streamFinished();
        } else {
            emit errorOccurred(tr("ffmpeg exited with code %1: %2")
                                   .arg(exitCode)
                                   .arg(QString::fromUtf8(m_process.readAllStandardError()).left(300)));
        }
    });
    connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (!m_stopping && error == QProcess::FailedToStart)
            emit errorOccurred(tr("ffmpeg could not start."));
    });
}

TranscodeSession::~TranscodeSession()
{
    stop();
}

QString TranscodeSession::ffmpegPath()
{
    return findTool(QStringLiteral("ffmpeg"));
}

QStringList TranscodeSession::encoderArgs()
{
    // Prefer platform-native H.264 encoders: they keep the ffmpeg build
    // LGPL-compatible (libx264 is GPL and only a dev-time fallback).
    const QString ffmpeg = ffmpegPath();
#ifdef Q_OS_MACOS
    if (!ffmpeg.isEmpty() && hasEncoder(ffmpeg, QStringLiteral("h264_videotoolbox")))
        return {QStringLiteral("-c:v"), QStringLiteral("h264_videotoolbox"),
                QStringLiteral("-b:v"), QStringLiteral("4M")};
#endif
    return {QStringLiteral("-c:v"), QStringLiteral("libx264"),
            QStringLiteral("-preset"), QStringLiteral("veryfast"),
            QStringLiteral("-tune"), QStringLiteral("zerolatency"),
            QStringLiteral("-b:v"), QStringLiteral("4M"),
            QStringLiteral("-maxrate"), QStringLiteral("4M"),
            QStringLiteral("-bufsize"), QStringLiteral("8M")};
}

bool TranscodeSession::start(const QString &filePath)
{
    stop();
    const QString tool = ffmpegPath();
    if (tool.isEmpty())
        return false;

    QStringList args;
    // -re paces input reading to native rate so the live stream delivers
    // smoothly (and memory stays bounded) instead of bursting ahead.
    args << QStringLiteral("-hide_banner") << QStringLiteral("-loglevel") << QStringLiteral("error")
         << QStringLiteral("-re")
         << QStringLiteral("-i") << filePath
         << encoderArgs()
         // Cap at 1080p: 4K H.264 is slow to encode and hard for DLNA TVs to
         // decode, a common cause of cast stutter. Keep aspect, even height.
         << QStringLiteral("-vf") << QStringLiteral("scale='min(1920,iw)':-2")
         << QStringLiteral("-pix_fmt") << QStringLiteral("yuv420p")
         << QStringLiteral("-c:a") << QStringLiteral("aac")
         << QStringLiteral("-b:a") << QStringLiteral("192k")
         << QStringLiteral("-ac") << QStringLiteral("2")
         << QStringLiteral("-f") << QStringLiteral("mpegts")
         << QStringLiteral("-mpegts_flags") << QStringLiteral("+resend_headers")
         << QStringLiteral("pipe:1");

    m_stopping = false;
    m_process.start(tool, args);
    return m_process.waitForStarted(5000);
}

void TranscodeSession::stop()
{
    if (m_process.state() == QProcess::NotRunning)
        return;
    m_stopping = true;
    m_process.kill();
    m_process.waitForFinished(3000);
}

bool TranscodeSession::isRunning() const
{
    return m_process.state() != QProcess::NotRunning;
}

QIODevice *TranscodeSession::stream()
{
    return &m_process;
}