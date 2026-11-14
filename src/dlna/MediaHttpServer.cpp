#include "MediaHttpServer.h"

#include "Logging.h"

#include <QFile>
#include <QFileInfo>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QProcess>
#include <QTcpSocket>

namespace {
constexpr qint64 kChunkSize = 256 * 1024;
constexpr int kMaxRequestBytes = 16 * 1024;

// Per-connection state: parses one request, then streams the file range.
class MediaConnection : public QObject
{
public:
    MediaConnection(QTcpSocket *socket, const QString &filePath,
                    const QString &mimeType, qint64 fileSize, QObject *parent)
        : QObject(parent), m_socket(socket), m_mimeType(mimeType), m_fileSize(fileSize)
    {
        m_file.setFileName(filePath);
        connect(m_socket, &QTcpSocket::readyRead, this, [this]() { onRequestData(); });
        connect(m_socket, &QTcpSocket::disconnected, this, &QObject::deleteLater);
    }

private:
    void onRequestData()
    {
        m_request += m_socket->readAll();
        const qsizetype headerEnd = m_request.indexOf("\r\n\r\n");
        if (headerEnd < 0) {
            if (m_request.size() > kMaxRequestBytes)
                m_socket->disconnectFromHost();
            return;
        }
        handleRequest(m_request.left(headerEnd));
    }

    void handleRequest(const QByteArray &request)
    {
        const QList<QByteArray> lines = request.split('\n');
        const QList<QByteArray> parts = lines.first().trimmed().split(' ');
        if (parts.size() < 2) {
            m_socket->disconnectFromHost();
            return;
        }
        const QByteArray method = parts.at(0);
        const QByteArray path = parts.at(1).split('?').first();

        if (path != "/media" || (method != "GET" && method != "HEAD")) {
            m_socket->write("HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n"
                            "Connection: close\r\n\r\n");
            m_socket->disconnectFromHost();
            return;
        }

        qint64 start = 0;
        qint64 end = m_fileSize - 1;
        bool partial = false;
        for (const QByteArray &line : lines) {
            const QByteArray trimmed = line.trimmed();
            if (trimmed.toLower().startsWith("range:")) {
                const QByteArray value = trimmed.mid(6).trimmed();
                if (!value.startsWith("bytes="))
                    continue;
                const QList<QByteArray> range = value.mid(6).split('-');
                if (range.size() != 2)
                    continue;
                bool okStart = false, okEnd = false;
                const qint64 reqStart = range.at(0).isEmpty() ? -1 : range.at(0).toLongLong(&okStart);
                const qint64 reqEnd = range.at(1).isEmpty() ? -1 : range.at(1).toLongLong(&okEnd);
                if (reqStart < 0 && reqEnd >= 0) {
                    // suffix range: last N bytes
                    start = qMax<qint64>(0, m_fileSize - qMin(reqEnd, m_fileSize));
                    end = m_fileSize - 1;
                } else if (okStart && reqStart >= 0) {
                    start = reqStart;
                    end = (okEnd && reqEnd >= 0) ? qMin(reqEnd, m_fileSize - 1) : m_fileSize - 1;
                } else {
                    continue;
                }
                if (start >= m_fileSize || start > end) {
                    m_socket->write(QStringLiteral("HTTP/1.1 416 Range Not Satisfiable\r\n"
                                                   "Content-Range: bytes */%1\r\n"
                                                   "Content-Length: 0\r\nConnection: close\r\n\r\n")
                                        .arg(m_fileSize).toUtf8());
                    m_socket->disconnectFromHost();
                    return;
                }
                partial = true;
            }
        }

        if (!m_file.open(QIODevice::ReadOnly) || !m_file.seek(start)) {
            m_socket->write("HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n"
                            "Connection: close\r\n\r\n");
            m_socket->disconnectFromHost();
            return;
        }
        m_remaining = end - start + 1;

        QByteArray header;
        header += partial ? "HTTP/1.1 206 Partial Content\r\n" : "HTTP/1.1 200 OK\r\n";
        header += "Content-Type: " + m_mimeType.toUtf8() + "\r\n";
        header += "Accept-Ranges: bytes\r\n";
        header += QStringLiteral("Content-Length: %1\r\n").arg(m_remaining).toUtf8();
        if (partial)
            header += QStringLiteral("Content-Range: bytes %1-%2/%3\r\n")
                          .arg(start).arg(end).arg(m_fileSize).toUtf8();
        // DLNA renderers (Dangbei/Lebo) expect these hints for streaming playback.
        header += "transferMode.dlna.org: Streaming\r\n";
        header += "realTimeInfo.dlna.org: DLNA.ORG_TLAG=*\r\n";
        header += "Connection: close\r\n\r\n";
        m_socket->write(header);

        if (method == "HEAD") {
            m_file.close();
            m_socket->disconnectFromHost();
            return;
        }

        connect(m_socket, &QTcpSocket::bytesWritten, this, [this]() { pump(); });
        pump();
    }

    void pump()
    {
        // Keep at most ~2 chunks in the socket buffer to bound memory use.
        while (m_remaining > 0 && m_socket->bytesToWrite() < kChunkSize) {
            const QByteArray chunk = m_file.read(qMin(kChunkSize, m_remaining));
            if (chunk.isEmpty()) {
                m_file.close();
                m_socket->disconnectFromHost();
                return;
            }
            m_remaining -= chunk.size();
            m_socket->write(chunk);
        }
        if (m_remaining == 0 && !m_finished) {
            m_finished = true;
            m_file.close();
            connect(m_socket, &QTcpSocket::bytesWritten, this, [this]() {
                if (m_socket->bytesToWrite() == 0)
                    m_socket->disconnectFromHost();
            });
            if (m_socket->bytesToWrite() == 0)
                m_socket->disconnectFromHost();
        }
    }

    QTcpSocket *m_socket;
    QFile m_file;
    QString m_mimeType;
    qint64 m_fileSize;
    qint64 m_remaining = 0;
    bool m_finished = false;
    QByteArray m_request;
};

// Streams a live transcode pipe to a single renderer: no Content-Length,
// no Range (the TV cannot seek a live transcode), body ends at EOF.
class LiveMediaConnection : public QObject
{
public:
    LiveMediaConnection(QTcpSocket *socket, QIODevice *stream, const QString &mimeType,
                        bool *busyFlag, QObject *parent)
        : QObject(parent), m_socket(socket), m_stream(stream),
          m_mimeType(mimeType), m_busyFlag(busyFlag)
    {
        connect(m_socket, &QTcpSocket::readyRead, this, [this]() { onRequestData(); });
        connect(m_socket, &QTcpSocket::disconnected, this, [this]() {
            qCInfo(lcHttp) << "live client disconnected" << m_socket->peerAddress().toString();
            *m_busyFlag = false;
            deleteLater();
        });
    }

private:
    void onRequestData()
    {
        m_request += m_socket->readAll();
        const qsizetype headerEnd = m_request.indexOf("\r\n\r\n");
        if (headerEnd < 0) {
            if (m_request.size() > kMaxRequestBytes)
                m_socket->disconnectFromHost();
            return;
        }
        const QList<QByteArray> parts = m_request.left(headerEnd).split('\n').first().trimmed().split(' ');
        qCInfo(lcHttp) << "live request:" << m_request.left(headerEnd).split('\n').first().trimmed()
                       << "from" << m_socket->peerAddress().toString();
        if (parts.size() < 2 || parts.at(1).split('?').first() != "/media"
            || (parts.at(0) != "GET" && parts.at(0) != "HEAD")) {
            m_socket->write("HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n"
                            "Connection: close\r\n\r\n");
            m_socket->disconnectFromHost();
            return;
        }
        if (*m_busyFlag) {
            qCWarning(lcHttp) << "live request rejected 503 (another client is streaming)";
            m_socket->write("HTTP/1.1 503 Service Unavailable\r\nContent-Length: 0\r\n"
                            "Connection: close\r\n\r\n");
            m_socket->disconnectFromHost();
            return;
        }
        *m_busyFlag = true;

        QByteArray header;
        header += "HTTP/1.1 200 OK\r\n";
        header += "Content-Type: " + m_mimeType.toUtf8() + "\r\n";
        header += "transferMode.dlna.org: Streaming\r\n";
        header += "realTimeInfo.dlna.org: DLNA.ORG_TLAG=*\r\n";
        header += "Connection: close\r\n\r\n";
        m_socket->write(header);

        if (parts.at(0) == "HEAD") {
            *m_busyFlag = false;
            m_socket->disconnectFromHost();
            return;
        }

        connect(m_stream, &QIODevice::readyRead, this, [this]() { pump(); });
        connect(m_socket, &QTcpSocket::bytesWritten, this, [this]() { pump(); });
        pump();
    }

    void pump()
    {
        while (m_socket->bytesToWrite() < kChunkSize && m_stream->bytesAvailable() > 0)
            m_socket->write(m_stream->read(kChunkSize));
        if (streamAtEnd() && m_socket->bytesToWrite() == 0)
            m_socket->disconnectFromHost();
    }

    // QProcess::atEnd() is true whenever its read buffer is empty, even while
    // running; only treat the stream as ended once the process has exited.
    bool streamAtEnd() const
    {
        if (auto *process = qobject_cast<QProcess *>(m_stream))
            return process->state() == QProcess::NotRunning && m_stream->bytesAvailable() == 0;
        return m_stream->atEnd();
    }

    QTcpSocket *m_socket;
    QIODevice *m_stream;
    QString m_mimeType;
    bool *m_busyFlag;
    QByteArray m_request;
};
} // namespace

MediaHttpServer::MediaHttpServer(QObject *parent)
    : QTcpServer(parent)
{
}

bool MediaHttpServer::start(const QString &filePath)
{
    stop();

    const QFileInfo info(filePath);
    if (!info.isFile())
        return false;
    m_filePath = info.absoluteFilePath();
    m_fileSize = info.size();
    m_mimeType = mimeTypeForFile(m_filePath);

    if (!listen(QHostAddress::AnyIPv4, 0)) {
        m_filePath.clear();
        return false;
    }
    return true;
}

bool MediaHttpServer::startLive(QIODevice *stream, const QString &mimeType)
{
    stop();
    if (!stream)
        return false;
    m_liveStream = stream;
    m_mimeType = mimeType;
    if (!listen(QHostAddress::AnyIPv4, 0)) {
        m_liveStream = nullptr;
        return false;
    }
    return true;
}

void MediaHttpServer::stop()
{
    if (isListening())
        close();
    const QList<QTcpSocket *> sockets = findChildren<QTcpSocket *>();
    for (QTcpSocket *socket : sockets)
        socket->abort();
    m_filePath.clear();
    m_fileSize = 0;
    m_liveStream = nullptr;
    m_liveBusy = false;
}

QUrl MediaHttpServer::mediaUrl() const
{
    if (!isListening())
        return {};

    // Pick the LAN IPv4 address the TV can reach: up, non-loopback,
    // non-link-local. Prefer physical en* (Wi-Fi/Ethernet) over virtual
    // interfaces (bridges, VPN utun) which the TV usually cannot reach.
    QList<QHostAddress> candidates;
    const QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface &iface : interfaces) {
        const auto flags = iface.flags();
        if (!(flags & QNetworkInterface::IsUp) || !(flags & QNetworkInterface::IsRunning)
            || (flags & QNetworkInterface::IsLoopBack))
            continue;
        const bool physical = iface.name().startsWith(QLatin1String("en"));
        const QList<QNetworkAddressEntry> entries = iface.addressEntries();
        for (const QNetworkAddressEntry &entry : entries) {
            const QHostAddress ip = entry.ip();
            if (ip.protocol() != QAbstractSocket::IPv4Protocol || ip.isLinkLocal())
                continue;
            if (physical)
                candidates.prepend(ip);
            else
                candidates.append(ip);
        }
    }
    if (candidates.isEmpty())
        return {};
    QUrl url;
    url.setScheme(QStringLiteral("http"));
    url.setHost(candidates.first().toString());
    url.setPort(serverPort());
    url.setPath(QStringLiteral("/media"));
    return url;
}

QString MediaHttpServer::filePath() const
{
    return m_filePath;
}

QString MediaHttpServer::mimeType() const
{
    return m_mimeType;
}

qint64 MediaHttpServer::fileSize() const
{
    return m_fileSize;
}

bool MediaHttpServer::isLive() const
{
    return m_liveStream != nullptr;
}

QString MediaHttpServer::mimeTypeForFile(const QString &filePath)
{
    const QString suffix = QFileInfo(filePath).suffix().toLower();
    if (suffix == QLatin1String("mp4") || suffix == QLatin1String("m4v"))
        return QStringLiteral("video/mp4");
    if (suffix == QLatin1String("mkv"))
        return QStringLiteral("video/x-matroska");
    if (suffix == QLatin1String("avi"))
        return QStringLiteral("video/x-msvideo");
    if (suffix == QLatin1String("mov"))
        return QStringLiteral("video/quicktime");
    if (suffix == QLatin1String("ts"))
        return QStringLiteral("video/mp2t");
    if (suffix == QLatin1String("webm"))
        return QStringLiteral("video/webm");
    if (suffix == QLatin1String("wmv"))
        return QStringLiteral("video/x-ms-wmv");
    if (suffix == QLatin1String("flv"))
        return QStringLiteral("video/x-flv");
    if (suffix == QLatin1String("mpg") || suffix == QLatin1String("mpeg"))
        return QStringLiteral("video/mpeg");
    return QStringLiteral("application/octet-stream");
}

void MediaHttpServer::incomingConnection(qintptr socketDescriptor)
{
    auto *socket = new QTcpSocket(this);
    if (!socket->setSocketDescriptor(socketDescriptor)) {
        delete socket;
        return;
    }
    if (m_liveStream)
        new LiveMediaConnection(socket, m_liveStream, m_mimeType, &m_liveBusy, socket);
    else
        new MediaConnection(socket, m_filePath, m_mimeType, m_fileSize, socket);
}