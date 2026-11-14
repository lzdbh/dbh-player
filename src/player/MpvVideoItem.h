#pragma once

#include <QQuickPaintedItem>
#include <QImage>
#include <QMutex>

#include <mpv/render.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

class MpvPlayer;

// QML video surface using libmpv's software render API. A dedicated render
// thread consumes mpv frames (the render API applies backpressure, so frames
// must be rendered off the GUI thread at playback cadence); the GUI thread
// only paints the latest finished frame.
class MpvVideoItem : public QQuickPaintedItem
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(MpvPlayer *player READ player WRITE setPlayer NOTIFY playerChanged)

public:
    explicit MpvVideoItem(QQuickItem *parent = nullptr);
    ~MpvVideoItem() override;

    MpvPlayer *player() const { return m_player; }
    void setPlayer(MpvPlayer *player);

    void paint(QPainter *painter) override;

    void wakeRender();

signals:
    void playerChanged();

private:
    void ensureRenderContext();
    void renderLoop();

    MpvPlayer *m_player = nullptr;
    mpv_render_context *m_renderCtx = nullptr;

    std::thread m_renderThread;
    std::mutex m_cvMutex;
    std::condition_variable m_cv;
    std::atomic<bool> m_quit{false};

    QMutex m_frameMutex;
    QImage m_frame;
    QByteArray m_buffer;
};