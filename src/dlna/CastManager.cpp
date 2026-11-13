#include "CastManager.h"

#include <QFileInfo>

CastManager::CastManager(QObject *parent)
    : QObject(parent)
{
    m_pollTimer.setInterval(1000);
    connect(&m_pollTimer, &QTimer::timeout, this, [this]() {
        m_avTransport.getPositionInfo();
        m_avTransport.getTransportInfo();
    });

    connect(&m_avTransport, &AvTransportClient::uriSet, this, [this]() {
        if (!m_active)
            return;
        m_avTransport.play();
        setPlaying(true);
        setStatus(m_transcoding ? QStringLiteral("Casting (transcoded)")
                                : QStringLiteral("Casting"));
        m_renderingControl.getVolume();
        m_pollTimer.start();
    });

    connect(&m_avTransport, &AvTransportClient::positionInfoReceived, this,
            [this](qint64 positionMs, qint64 durationMs) {
                if (!m_active)
                    return;
                if (m_position != positionMs) {
                    m_position = positionMs;
                    emit positionChanged();
                }
                if (durationMs > 0 && m_duration != durationMs) {
                    m_duration = durationMs;
                    emit durationChanged();
                }
            });

    connect(&m_avTransport, &AvTransportClient::transportStateChanged, this,
            [this](const QString &state) {
                if (!m_active)
                    return;
                if (state == QLatin1String("PLAYING")) {
                    setPlaying(true);
                } else if (state == QLatin1String("PAUSED_PLAYBACK")) {
                    setPlaying(false);
                } else if (state == QLatin1String("STOPPED")) {
                    setStatus(QStringLiteral("Stopped on device"));
                    disconnect();
                }
            });

    connect(&m_avTransport, &AvTransportClient::actionFailed, this,
            [this](const QString &action, const QString &error) {
                if (!m_active)
                    return;
                // Polling failures are transient; setup/control failures end the session.
                if (action == QLatin1String("GetPositionInfo")
                    || action == QLatin1String("GetTransportInfo"))
                    return;
                fail(QStringLiteral("%1 failed: %2").arg(action, error));
            });

    connect(&m_renderingControl, &RenderingControlClient::volumeReceived, this,
            [this](int volume) {
                if (m_volume != volume) {
                    m_volume = volume;
                    emit volumeChanged();
                }
            });

    connect(&m_probe, &MediaProbe::probed, this, [this](const MediaProbe::Result &result) {
        if (!m_active)
            return;
        if (!result.valid) {
            fail(result.error);
            return;
        }
        const QString title = QFileInfo(m_pendingPath).fileName();
        if (result.directPushCompatible) {
            if (!m_server.start(m_pendingPath)) {
                fail(QStringLiteral("Cannot start local media server."));
                return;
            }
            beginPush(m_server.mediaUrl(), title, m_server.mimeType(), m_server.fileSize(), false);
        } else {
            if (TranscodeSession::ffmpegPath().isEmpty()) {
                fail(QStringLiteral("This format needs transcoding but ffmpeg was not found. "
                                    "Install ffmpeg to cast %1 files.")
                         .arg(result.formatName));
                return;
            }
            setStatus(QStringLiteral("Transcoding for %1...").arg(m_deviceName));
            if (!m_transcode.start(m_pendingPath)) {
                fail(QStringLiteral("Cannot start ffmpeg transcode."));
                return;
            }
            if (!m_server.startLive(m_transcode.stream(), QStringLiteral("video/mp2t"))) {
                m_transcode.stop();
                fail(QStringLiteral("Cannot start local media server."));
                return;
            }
            setTranscoding(true);
            if (result.durationMs > 0) {
                m_duration = result.durationMs;
                emit durationChanged();
            }
            beginPush(m_server.mediaUrl(), title, QStringLiteral("video/mp2t"), 0, true);
        }
    });

    connect(&m_transcode, &TranscodeSession::errorOccurred, this, [this](const QString &message) {
        if (m_active)
            fail(message);
    });
}

bool CastManager::active() const { return m_active; }
QString CastManager::deviceName() const { return m_deviceName; }
bool CastManager::playing() const { return m_playing; }
qint64 CastManager::position() const { return m_position; }
qint64 CastManager::duration() const { return m_duration; }
int CastManager::volume() const { return m_volume; }
QString CastManager::status() const { return m_status; }

void CastManager::cast(const QUrl &avTransportUrl, const QUrl &renderingControlUrl,
                       const QString &deviceName, const QUrl &localFile)
{
    const QString path = localFile.toLocalFile();
    if (path.isEmpty() || !QFileInfo::exists(path)) {
        emit castFailed(QStringLiteral("Open a local video file before casting."));
        return;
    }
    if (!avTransportUrl.isValid()) {
        emit castFailed(QStringLiteral("Device has no AVTransport control URL."));
        return;
    }

    if (m_active)
        disconnect();

    m_pendingPath = path;
    m_pendingAvt = avTransportUrl;
    m_pendingRc = renderingControlUrl;
    m_deviceName = deviceName;
    emit deviceNameChanged();
    m_position = 0;
    m_duration = 0;
    emit positionChanged();
    emit durationChanged();

    m_avTransport.setControlUrl(avTransportUrl);
    m_renderingControl.setControlUrl(renderingControlUrl);

    setActive(true);
    setStatus(QStringLiteral("Checking format..."));
    m_probe.probe(path);
    emit castStarted();
}

void CastManager::beginPush(const QUrl &mediaUrl, const QString &title,
                            const QString &mimeType, qint64 sizeBytes, bool live)
{
    if (!mediaUrl.isValid()) {
        fail(QStringLiteral("No LAN IPv4 address found; is Wi-Fi connected?"));
        return;
    }
    setStatus(m_transcoding ? QStringLiteral("Casting (transcoded)")
                            : QStringLiteral("Connecting to %1...").arg(m_deviceName));
    m_avTransport.setUri(mediaUrl, title, mimeType, sizeBytes, live);
}

void CastManager::play()
{
    if (!m_active)
        return;
    m_avTransport.play();
    setPlaying(true);
}

void CastManager::pause()
{
    if (!m_active)
        return;
    m_avTransport.pause();
    setPlaying(false);
}

void CastManager::togglePlay()
{
    m_playing ? pause() : play();
}

void CastManager::seek(qint64 positionMs)
{
    if (!m_active || m_transcoding)
        return; // live transcode stream cannot seek
    m_avTransport.seek(positionMs);
    m_position = positionMs;
    emit positionChanged();
}

void CastManager::setVolume(int volume)
{
    if (!m_active)
        return;
    m_renderingControl.setVolume(volume);
    m_volume = qBound(0, volume, 100);
    emit volumeChanged();
}

void CastManager::disconnect()
{
    if (!m_active)
        return;
    m_pollTimer.stop();
    m_avTransport.stop();
    m_server.stop();
    m_transcode.stop();
    setTranscoding(false);
    setPlaying(false);
    setActive(false);
    setStatus(QStringLiteral("Disconnected"));
}

void CastManager::setActive(bool active)
{
    if (m_active == active)
        return;
    m_active = active;
    emit activeChanged();
}

void CastManager::setPlaying(bool playing)
{
    if (m_playing == playing)
        return;
    m_playing = playing;
    emit playingChanged();
}

void CastManager::setStatus(const QString &status)
{
    if (m_status == status)
        return;
    m_status = status;
    emit statusChanged();
}

void CastManager::fail(const QString &message)
{
    m_pollTimer.stop();
    m_server.stop();
    m_transcode.stop();
    setTranscoding(false);
    setPlaying(false);
    setActive(false);
    setStatus(message);
    emit castFailed(message);
}

bool CastManager::transcoding() const { return m_transcoding; }

void CastManager::setTranscoding(bool transcoding)
{
    if (m_transcoding == transcoding)
        return;
    m_transcoding = transcoding;
    emit transcodingChanged();
}