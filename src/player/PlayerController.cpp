#include "PlayerController.h"

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
    m_player->setAudioOutput(m_audio);

    connect(m_player, &QMediaPlayer::playbackStateChanged, this,
            [this](QMediaPlayer::PlaybackState) { emit playingChanged(); });
    connect(m_player, &QMediaPlayer::positionChanged, this, &PlayerController::positionChanged);
    connect(m_player, &QMediaPlayer::durationChanged, this, &PlayerController::durationChanged);
    connect(m_player, &QMediaPlayer::hasVideoChanged, this, &PlayerController::hasVideoChanged);
    connect(m_audio, &QAudioOutput::volumeChanged, this, &PlayerController::volumeChanged);
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

float PlayerController::playbackRate() const
{
    return m_player ? m_player->playbackRate() : 1.0f;
}

void PlayerController::setPlaybackRate(float rate)
{
    if (m_player)
        m_player->setPlaybackRate(rate);
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