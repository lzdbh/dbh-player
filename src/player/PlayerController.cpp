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
        // Embedded subtitle tracks found: list them for selection and disable
        // Qt's baked-in rendering (opaque background); we extract to SRT and
        // render with our own transparent, outlined style instead.
        const auto subs = m_player->subtitleTracks();
        if (!subs.isEmpty() && !m_subtitleProbeDone) {
            m_subtitleProbeDone = true;
            m_player->setActiveSubtitleTrack(-1);
            probeEmbeddedSubtitles();
        }
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
    m_subtitleProbeDone = false;
    m_subtitleTracks.clear();
    m_extractedSubtitlePath.clear();
    if (m_selectedSubtitleIndex != -1) {
        m_selectedSubtitleIndex = -1;
        emit selectedSubtitleIndexChanged();
    }
    emit subtitleTracksChanged();
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

QVariantList PlayerController::subtitleTracks() const
{
    return m_subtitleTracks;
}

void PlayerController::disableSubtitles()
{
    if (m_player)
        m_player->setActiveSubtitleTrack(-1);
    if (m_selectedSubtitleIndex != -1) {
        m_selectedSubtitleIndex = -1;
        emit selectedSubtitleIndexChanged();
    }
    emit subtitlesCleared();
}

void PlayerController::selectSubtitleTrack(int streamIndex)
{
    if (m_selectedSubtitleIndex != streamIndex) {
        m_selectedSubtitleIndex = streamIndex;
        emit selectedSubtitleIndexChanged();
    }
    extractSubtitleStream(streamIndex);
}

// Lists embedded subtitle streams (via ffprobe) for the UI to choose from and
// auto-selects the Chinese one.
void PlayerController::probeEmbeddedSubtitles()
{
    const QString file = m_player->source().toLocalFile();
    const QString ffprobe = QStandardPaths::findExecutable(QStringLiteral("ffprobe"));
    if (file.isEmpty() || ffprobe.isEmpty())
        return;

    auto *probe = new QProcess(this);
    connect(probe, &QProcess::finished, this, [this, probe](int) {
        const QString out = QString::fromUtf8(probe->readAllStandardOutput());
        probe->deleteLater();
        m_subtitleTracks.clear();
        int chinese = -1;
        const QStringList lines = out.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
        for (const QString &line : lines) {
            const QStringList parts = line.trimmed().split(QLatin1Char(','));
            if (parts.size() < 2)
                continue;
            const int idx = parts.at(0).toInt();
            const QString lang = parts.at(1).trimmed();
            const QString title = parts.size() >= 3 ? parts.mid(2).join(QLatin1Char(',')).trimmed()
                                                    : QString();
            const QString label = !title.isEmpty() ? title
                                  : lang.isEmpty() ? QStringLiteral("Track %1").arg(idx) : lang;
            m_subtitleTracks.append(QVariantMap{{QStringLiteral("index"), idx},
                                                {QStringLiteral("label"), label}});
            const QString lower = lang.toLower();
            if (chinese < 0 && (lower.startsWith(QLatin1String("chi"))
                                || lower.startsWith(QLatin1String("zh"))))
                chinese = idx;
        }
        qCInfo(lcPlayer) << "subtitle tracks found:" << m_subtitleTracks.size();
        emit subtitleTracksChanged();
        if (!m_subtitleTracks.isEmpty()) {
            const int pick = chinese >= 0 ? chinese
                                          : m_subtitleTracks.first().toMap().value(QStringLiteral("index")).toInt();
            m_selectedSubtitleIndex = pick;
            emit selectedSubtitleIndexChanged();
            extractSubtitleStream(pick);
        }
    });
    probe->start(ffprobe, {QStringLiteral("-v"), QStringLiteral("quiet"),
                           QStringLiteral("-select_streams"), QStringLiteral("s"),
                           QStringLiteral("-show_entries"), QStringLiteral("stream=index:stream_tags=language,title"),
                           QStringLiteral("-of"), QStringLiteral("csv=p=0"), file});
}

void PlayerController::extractSubtitleStream(int streamIndex)
{
    const QString file = m_player->source().toLocalFile();
    const QString ffmpeg = QStandardPaths::findExecutable(QStringLiteral("ffmpeg"));
    if (file.isEmpty() || ffmpeg.isEmpty() || streamIndex < 0)
        return;
    const QString outPath = QDir::tempPath()
                            + QStringLiteral("/dbhx-subtitle-%1.srt").arg(streamIndex);
    auto *ex = new QProcess(this);
    connect(ex, &QProcess::finished, this, [this, ex, outPath](int rc) {
        ex->deleteLater();
        if (rc == 0 && QFileInfo::exists(outPath))
            emit subtitleFileReady(QUrl::fromLocalFile(outPath));
        else
            qCWarning(lcPlayer) << "subtitle extraction failed:"
                                << ex->readAllStandardError().left(200);
    });
    connect(ex, &QProcess::errorOccurred, this, [](QProcess::ProcessError e) {
        qCWarning(lcPlayer) << "subtitle ffmpeg process error" << e;
    });
    m_extractedSubtitlePath = outPath;
    ex->start(ffmpeg, {QStringLiteral("-hide_banner"), QStringLiteral("-loglevel"),
                       QStringLiteral("error"), QStringLiteral("-y"),
                       QStringLiteral("-i"), file,
                       QStringLiteral("-map"), QStringLiteral("0:%1").arg(streamIndex),
                       QStringLiteral("-c:s"), QStringLiteral("srt"), outPath});
}