#include "MpvPlayer.h"

#include "Logging.h"

#include <QStringList>
#include <QVariant>

MpvPlayer::MpvPlayer(QObject *parent)
    : QObject(parent)
{
    m_mpv = mpv_create();
    if (!m_mpv) {
        qCCritical(lcPlayer) << "mpv_create failed";
        return;
    }

    auto opt = [this](const char *name, const char *value) {
        mpv_set_option_string(m_mpv, name, value);
    };
    opt("vo", "libmpv");
    opt("hwdec", "auto-safe");
    // The default coreaudio buffer is too small on macOS and makes the audio
    // clock run slow (~0.8x), dragging all playback. A larger buffer fixes it.
    opt("audio-buffer", "0.5");
    opt("idle", "yes");
    opt("keep-open", "no");
    opt("osc", "no");
    opt("osd-level", "0");
    opt("sub-auto", "no");
    opt("sub-visibility", "no");
    opt("cache", "yes");
    opt("cache-secs", "30");
    opt("demuxer-max-bytes", "64MiB");
    opt("input-default-bindings", "no");
    opt("input-vo-keyboard", "no");

    mpv_request_log_messages(m_mpv, "info");

    if (mpv_initialize(m_mpv) < 0) {
        qCCritical(lcPlayer) << "mpv_initialize failed";
        mpv_destroy(m_mpv);
        m_mpv = nullptr;
        return;
    }

    mpv_observe_property(m_mpv, 0, "time-pos", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_mpv, 0, "duration", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_mpv, 0, "pause", MPV_FORMAT_FLAG);
    mpv_observe_property(m_mpv, 0, "volume", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_mpv, 0, "mute", MPV_FORMAT_FLAG);
    mpv_observe_property(m_mpv, 0, "speed", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_mpv, 0, "dwidth", MPV_FORMAT_INT64);

    double vol = m_volume * 100.0;
    mpv_set_property(m_mpv, "volume", MPV_FORMAT_DOUBLE, &vol);

    // Drain mpv's event queue on a timer, independent of the wakeup callback.
    m_eventTimer.setInterval(10);
    connect(&m_eventTimer, &QTimer::timeout, this, &MpvPlayer::pollEvents);
    m_eventTimer.start();
}

MpvPlayer::~MpvPlayer()
{
    m_eventTimer.stop();
    if (m_mpv) {
        mpv_terminate_destroy(m_mpv);
        m_mpv = nullptr;
    }
}

void MpvPlayer::command(const QStringList &args)
{
    if (!m_mpv)
        return;
    QList<QByteArray> utf8;
    QList<const char *> argv;
    for (const QString &a : args) {
        utf8.append(a.toUtf8());
        argv.append(utf8.last().constData());
    }
    argv.append(nullptr);
    mpv_command(m_mpv, argv.data());
}

void MpvPlayer::setMpvProperty(const QString &name, const QVariant &value)
{
    if (!m_mpv)
        return;
    const QByteArray key = name.toUtf8();
    if (value.typeId() == QMetaType::Bool) {
        int flag = value.toBool() ? 1 : 0;
        mpv_set_property(m_mpv, key.constData(), MPV_FORMAT_FLAG, &flag);
    } else {
        double d = value.toDouble();
        mpv_set_property(m_mpv, key.constData(), MPV_FORMAT_DOUBLE, &d);
    }
}

void MpvPlayer::pollEvents()
{
    if (!m_mpv)
        return;
    while (true) {
        mpv_event *event = mpv_wait_event(m_mpv, 0);
        if (!event || event->event_id == MPV_EVENT_NONE)
            break;

        switch (event->event_id) {
        case MPV_EVENT_PROPERTY_CHANGE: {
            auto *prop = static_cast<mpv_event_property *>(event->data);
            if (!prop || prop->format == MPV_FORMAT_NONE || !prop->data)
                break;
            const QString name = QString::fromUtf8(prop->name);
            if (name == QLatin1String("time-pos") && prop->format == MPV_FORMAT_DOUBLE) {
                m_position = qint64(*static_cast<double *>(prop->data) * 1000);
                if (m_position / 1000 != m_lastLogPos / 1000) {
                    m_lastLogPos = m_position;
                    qCInfo(lcPlayer) << "POS" << m_position;
                }
                emit positionChanged();
            } else if (name == QLatin1String("duration") && prop->format == MPV_FORMAT_DOUBLE) {
                m_duration = qint64(*static_cast<double *>(prop->data) * 1000);
                emit durationChanged();
            } else if (name == QLatin1String("pause") && prop->format == MPV_FORMAT_FLAG) {
                m_playing = !*static_cast<int *>(prop->data);
                emit playingChanged();
            } else if (name == QLatin1String("volume") && prop->format == MPV_FORMAT_DOUBLE) {
                m_volume = float(*static_cast<double *>(prop->data) / 100.0);
                emit volumeChanged();
            } else if (name == QLatin1String("mute") && prop->format == MPV_FORMAT_FLAG) {
                m_muted = *static_cast<int *>(prop->data);
                emit mutedChanged();
            } else if (name == QLatin1String("speed") && prop->format == MPV_FORMAT_DOUBLE) {
                m_rate = float(*static_cast<double *>(prop->data));
                emit playbackRateChanged();
            } else if (name == QLatin1String("dwidth") && prop->format == MPV_FORMAT_INT64) {
                m_hasVideo = *static_cast<int64_t *>(prop->data) > 0;
                emit hasVideoChanged();
            }
            break;
        }
        case MPV_EVENT_END_FILE: {
            auto *ef = static_cast<mpv_event_end_file *>(event->data);
            if (ef && ef->reason == MPV_END_FILE_REASON_EOF) {
                m_playing = false;
                emit playingChanged();
                emit endOfMedia();
            }
            break;
        }
        case MPV_EVENT_LOG_MESSAGE: {
            auto *log = static_cast<mpv_event_log_message *>(event->data);
            if (log && log->text)
                qCDebug(lcPlayer) << "mpv:" << log->text;
            break;
        }
        default:
            break;
        }
    }
}

QUrl MpvPlayer::source() const { return m_source; }

void MpvPlayer::setSource(const QUrl &url)
{
    if (m_source == url)
        return;
    m_source = url;
    emit sourceChanged();
}

bool MpvPlayer::playing() const { return m_playing; }
qint64 MpvPlayer::position() const { return m_position; }
qint64 MpvPlayer::duration() const { return m_duration; }
float MpvPlayer::volume() const { return m_volume; }
bool MpvPlayer::muted() const { return m_muted; }
float MpvPlayer::playbackRate() const { return m_rate; }
bool MpvPlayer::hasVideo() const { return m_hasVideo; }

void MpvPlayer::setVolume(float volume)
{
    volume = qBound(0.0f, volume, 1.0f);
    if (qFuzzyCompare(m_volume, volume))
        return;
    m_volume = volume;
    setMpvProperty(QStringLiteral("volume"), double(volume * 100.0));
    emit volumeChanged();
}

void MpvPlayer::setMuted(bool muted)
{
    if (m_muted == muted)
        return;
    m_muted = muted;
    setMpvProperty(QStringLiteral("mute"), muted);
    emit mutedChanged();
}

void MpvPlayer::setPlaybackRate(float rate)
{
    if (qFuzzyCompare(m_rate, rate))
        return;
    m_rate = rate;
    setMpvProperty(QStringLiteral("speed"), double(rate));
    emit playbackRateChanged();
}

void MpvPlayer::loadPending()
{
    if (!m_source.isValid())
        return;
    const QString path = m_source.isLocalFile() ? m_source.toLocalFile() : m_source.toString();
    command({QStringLiteral("loadfile"), path, QStringLiteral("replace")});
    setMpvProperty(QStringLiteral("pause"), false);
}

void MpvPlayer::open(const QUrl &url)
{
    setSource(url);
    if (m_renderReady)
        loadPending();
}

void MpvPlayer::setRenderReady()
{
    if (m_renderReady)
        return;
    m_renderReady = true;
    loadPending();
}

void MpvPlayer::play() { setMpvProperty(QStringLiteral("pause"), false); }
void MpvPlayer::pause() { setMpvProperty(QStringLiteral("pause"), true); }
void MpvPlayer::togglePlay() { setMpvProperty(QStringLiteral("pause"), m_playing); }
void MpvPlayer::stop() { command({QStringLiteral("stop")}); }

void MpvPlayer::seek(qint64 positionMs)
{
    command({QStringLiteral("seek"), QString::number(positionMs / 1000.0), QStringLiteral("absolute")});
}