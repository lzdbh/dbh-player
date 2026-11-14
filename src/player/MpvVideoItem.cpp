#include "MpvVideoItem.h"
#include "MpvPlayer.h"
#include "Logging.h"

#include <QPainter>
#include <QMetaObject>

namespace {
void swUpdateCallback(void *ctx)
{
    static_cast<MpvVideoItem *>(ctx)->wakeRender();
}
} // namespace

MpvVideoItem::MpvVideoItem(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    setOpaquePainting(true);
}

MpvVideoItem::~MpvVideoItem()
{
    m_quit = true;
    m_cv.notify_all();
    if (m_renderThread.joinable())
        m_renderThread.join();
    if (m_renderCtx) {
        mpv_render_context_set_update_callback(m_renderCtx, nullptr, nullptr);
        mpv_render_context_free(m_renderCtx);
        m_renderCtx = nullptr;
    }
}

void MpvVideoItem::wakeRender()
{
    m_cv.notify_one();
}

void MpvVideoItem::setPlayer(MpvPlayer *player)
{
    if (m_player == player)
        return;
    m_player = player;
    emit playerChanged();
    ensureRenderContext();
}

void MpvVideoItem::ensureRenderContext()
{
    if (m_renderCtx || !m_player || !m_player->handle() || m_renderThread.joinable())
        return;
    m_renderThread = std::thread([this]() { renderLoop(); });
}

void MpvVideoItem::renderLoop()
{
    mpv_render_param params[] = {
        {MPV_RENDER_PARAM_API_TYPE, const_cast<char *>(MPV_RENDER_API_TYPE_SW)},
        {MPV_RENDER_PARAM_INVALID, nullptr},
    };
    if (mpv_render_context_create(&m_renderCtx, m_player->handle(), params) < 0) {
        qCWarning(lcPlayer) << "mpv SW render context create failed";
        m_renderCtx = nullptr;
        return;
    }
    mpv_render_context_set_update_callback(m_renderCtx, swUpdateCallback, this);
    QMetaObject::invokeMethod(m_player, "setRenderReady", Qt::QueuedConnection);

    while (!m_quit) {
        {
            std::unique_lock<std::mutex> lock(m_cvMutex);
            m_cv.wait_for(lock, std::chrono::milliseconds(16));
        }
        if (m_quit)
            break;
        mpv_render_context_update(m_renderCtx);

        const int w = qMax(1, qMin(960, int(width())));
        const int h = qMax(1, int(height()) * w / qMax(1, int(width())));
        size_t stride = size_t(w) * 4;
        m_buffer.resize(int(stride * h));
        int size[2] = {w, h};
        mpv_render_param rp[] = {
            {MPV_RENDER_PARAM_SW_SIZE, size},
            {MPV_RENDER_PARAM_SW_FORMAT, const_cast<char *>("rgba")},
            {MPV_RENDER_PARAM_SW_STRIDE, &stride},
            {MPV_RENDER_PARAM_SW_POINTER, m_buffer.data()},
            {MPV_RENDER_PARAM_INVALID, nullptr},
        };
        if (mpv_render_context_render(m_renderCtx, rp) < 0)
            continue;

        QImage frame(reinterpret_cast<const uchar *>(m_buffer.constData()),
                     w, h, int(stride), QImage::Format_RGBA8888);
        {
            QMutexLocker locker(&m_frameMutex);
            m_frame = frame.copy();
        }
        QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection);
    }
}

void MpvVideoItem::paint(QPainter *painter)
{
    QMutexLocker locker(&m_frameMutex);
    if (m_frame.isNull()) {
        painter->fillRect(boundingRect(), Qt::black);
        return;
    }
    painter->drawImage(boundingRect(), m_frame);
}