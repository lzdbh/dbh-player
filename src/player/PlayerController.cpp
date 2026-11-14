#include "PlayerController.h"

#include "Logging.h"

#include <QMediaMetaData>

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
        // Embedded subtitle tracks (e.g. muxed into MKV) appear here once the
        // media is loaded. Prefer a Chinese track; otherwise use the first.
        const auto subs = m_player->subtitleTracks();
        if (!subs.isEmpty() && m_player->activeSubtitleTrack() < 0) {
            int chosen = 0;
            for (int i = 0; i < subs.size(); ++i) {
                const QString lang = subs.at(i).stringValue(QMediaMetaData::Language);
                if (lang.startsWith(QLatin1String("zh"), Qt::CaseInsensitive)
                    || lang.startsWith(QLatin1String("chi"), Qt::CaseInsensitive)) {
                    chosen = i;
                    break;
                }
            }
            qCInfo(lcPlayer) << "subtitle tracks:" << subs.size() << "chosen" << chosen
                             << "lang" << subs.at(chosen).stringValue(QMediaMetaData::Language)
                             << "title" << subs.at(chosen).stringValue(QMediaMetaData::Title);
            m_player->setActiveSubtitleTrack(chosen);
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