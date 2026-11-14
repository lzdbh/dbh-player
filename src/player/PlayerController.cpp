#include "PlayerController.h"

#include "Logging.h"

#include <QDir>
#include <QFileInfo>
#include <QMediaMetaData>
#include <QProcess>
#include <QStandardPaths>

PlayerController::PlayerController(QObject *parent)
    : QObject(parent)
{
}

void PlayerController::ensurePlayer()
{
    if (m_player)
        return;

    m_player = new QMediaPlayer(this);
    m_audio = new QAudioOutput(this);
    m_audio->setVolume(m_volume);
    m_audio->setMuted(m_muted);
    m_player->setAudioOutput(m_audio);
    m_player->setPlaybackRate(m_rate);

    connect(m_player, &QMediaPlayer::playbackStateChanged, this,
            [this](QMediaPlayer::PlaybackState) { emit playingChanged(); });
    connect(m_player, &QMediaPlayer::positionChanged, this, &PlayerController::positionChanged);
    connect(m_player, &QMediaPlayer::durationChanged, this, &PlayerController::durationChanged);
    connect(m_player, &QMediaPlayer::hasVideoChanged, this, &PlayerController::hasVideoChanged);
    connect(m_player, &QMediaPlayer::playbackRateChanged, this, &PlayerController::playbackRateChanged);
    connect(m_audio, &QAudioOutput::volumeChanged, this, &PlayerController::volumeChanged);
    connect(m_audio, &QAudioOutput::mutedChanged, this, &PlayerController::mutedChanged);
    connect(m_player, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::EndOfMedia)
            emit endOfMedia();
    });
    connect(m_player, &QMediaPlayer::tracksChanged, this, [this]() {
        // For embedded subtitle tracks we disable Qt's baked-in rendering
        // (opaque background) and instead extract the Chinese track to SRT so
        // the UI renders it with its own transparent, outlined style.
        const auto subs = m_player->subtitleTracks();
        if (!subs.isEmpty() && !m_subtitleExtractDone) {
            m_subtitleExtractDone = true;
            m_player->setActiveSubtitleTrack(-1);
            extractEmbeddedSubtitle();
        }
    });
    connect(m_player, &QMediaPlayer::tracksChanged, this, [this]() {
        qCInfo(lcPlayer) << "tracksChanged: subtitleTracks=" << m_player->subtitleTracks().size()
                         << "active=" << m_player->activeSubtitleTrack();
    });
    connect(m_player, &QMediaPlayer::errorOccurred, this,
            [this](QMediaPlayer::Error, const QString &message) { emit errorOccurred(message); });

    emit mediaPlayerChanged();
    emit sourceChanged();
    emit playingChanged();
    emit positionChanged();
    emit durationChanged();
    emit volumeChanged();
    emit hasVideoChanged();
}

QMediaPlayer *PlayerController::mediaPlayer() const
{
    return m_player;
}

QUrl PlayerController::source() const
{
    return m_player ? m_player->source() : QUrl();
}

void PlayerController::setSource(const QUrl &url)
{
    ensurePlayer();
    if (m_player->source() == url)
        return;
    m_subtitleExtractDone = false;
    m_player->setSource(url);
    emit sourceChanged();
}

bool PlayerController::playing() const
{
    return m_player && m_player->playbackState() == QMediaPlayer::PlayingState;
}

qint64 PlayerController::position() const
{
    return m_player ? m_player->position() : 0;
}

qint64 PlayerController::duration() const
{
    return m_player ? m_player->duration() : 0;
}

float PlayerController::volume() const
{
    return m_audio ? m_audio->volume() : m_volume;
}

void PlayerController::setVolume(float volume)
{
    m_volume = volume;
    if (m_audio)
        m_audio->setVolume(volume);
}

bool PlayerController::muted() const
{
    return m_audio ? m_audio->isMuted() : m_muted;
}

void PlayerController::setMuted(bool muted)
{
    m_muted = muted;
    if (m_audio)
        m_audio->setMuted(muted);
}

float PlayerController::playbackRate() const
{
    return m_player ? m_player->playbackRate() : m_rate;
}

void PlayerController::setPlaybackRate(float rate)
{
    m_rate = rate;
    if (m_player)
        m_player->setPlaybackRate(rate);
    else
        emit playbackRateChanged();
}

bool PlayerController::hasVideo() const
{
    return m_player && m_player->hasVideo();
}

void PlayerController::open(const QUrl &url)
{
    setSource(url);
    play();
}

void PlayerController::play()
{
    ensurePlayer();
    m_player->play();
}

void PlayerController::pause()
{
    if (m_player)
        m_player->pause();
}

void PlayerController::togglePlay()
{
    if (playing())
        pause();
    else
        play();
}

void PlayerController::stop()
{
    if (m_player)
        m_player->stop();
}

void PlayerController::seek(qint64 positionMs)
{
    if (m_player)
        m_player->setPosition(positionMs);
}

// Extracts the Chinese embedded subtitle track to a temp SRT (via ffprobe +
// ffmpeg) and emits subtitleFileReady so the UI renders it with its own style.
void PlayerController::extractEmbeddedSubtitle()
{
    const QString file = m_player->source().toLocalFile();
    const QString ffprobe = QStandardPaths::findExecutable(QStringLiteral("ffprobe"));
    const QString ffmpeg = QStandardPaths::findExecutable(QStringLiteral("ffmpeg"));
    if (file.isEmpty() || ffprobe.isEmpty() || ffmpeg.isEmpty())
        return;
    qCInfo(lcPlayer) << "subtitle tools: ffprobe=" << ffprobe << "ffmpeg=" << ffmpeg;

    auto *probe = new QProcess(this);
    connect(probe, &QProcess::finished, this, [this, probe, file, ffmpeg](int) {
        const QString out = QString::fromUtf8(probe->readAllStandardOutput());
        probe->deleteLater();
        int chinese = -1, first = -1;
        const QStringList lines = out.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
        for (const QString &line : lines) {
            const QStringList parts = line.trimmed().split(QLatin1Char(','));
            if (parts.size() < 2)
                continue;
            const int idx = parts.at(0).toInt();
            const QString lang = parts.at(1).trimmed().toLower();
            if (first < 0)
                first = idx;
            if (chinese < 0 && (lang.startsWith(QLatin1String("chi"))
                                || lang.startsWith(QLatin1String("zh")))) {
                chinese = idx;
            }
        }
        const int track = chinese >= 0 ? chinese : first;
        if (track < 0)
            return;
        qCInfo(lcPlayer) << "extracting embedded subtitle stream" << track << "from" << file;
        const QString outPath = QDir::tempPath() + QStringLiteral("/dbhx-subtitle.srt");
        auto *ex = new QProcess(this);
        connect(ex, &QProcess::finished, this, [this, ex, outPath](int rc) {
            ex->deleteLater();
            if (rc == 0 && QFileInfo::exists(outPath)) {
                emit subtitleFileReady(QUrl::fromLocalFile(outPath));
            } else {
                qCWarning(lcPlayer) << "subtitle extraction failed:" << ex->readAllStandardError().left(200);
            }
        });
        ex->start(ffmpeg, {QStringLiteral("-hide_banner"), QStringLiteral("-loglevel"),
                           QStringLiteral("error"), QStringLiteral("-y"),
                           QStringLiteral("-i"), file,
                           QStringLiteral("-map"), QStringLiteral("0:%1").arg(track),
                           QStringLiteral("-c:s"), QStringLiteral("srt"), outPath});
        connect(ex, &QProcess::errorOccurred, this, [](QProcess::ProcessError e) {
            qCWarning(lcPlayer) << "subtitle ffmpeg process error" << e;
        });
    });
    probe->start(ffprobe, {QStringLiteral("-v"), QStringLiteral("quiet"),
                           QStringLiteral("-select_streams"), QStringLiteral("s"),
                           QStringLiteral("-show_entries"), QStringLiteral("stream=index:stream_tags=language"),
                           QStringLiteral("-of"), QStringLiteral("csv=p=0"), file});
}