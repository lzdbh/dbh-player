#include "SsdpDiscovery.h"

#include <QHostAddress>
#include <QNetworkInterface>
#include <QTimer>

namespace {
const char *kMulticastAddr = "239.255.255.250";
constexpr quint16 kMulticastPort = 1900;
} // namespace

SsdpDiscovery::SsdpDiscovery(QObject *parent)
    : QObject(parent)
{
    connect(&m_socket, &QUdpSocket::readyRead, this, &SsdpDiscovery::onReadyRead);
}

void SsdpDiscovery::startSearch()
{
    if (!m_bound) {
        m_bound = m_socket.bind(QHostAddress::AnyIPv4, kMulticastPort,
                                QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
        if (!m_bound)
            m_bound = m_socket.bind(QHostAddress::AnyIPv4, 0);
        m_socket.joinMulticastGroup(QHostAddress(kMulticastAddr));
    }

    // Send a few probes so devices that miss the first one still respond.
    sendSearch();
    QTimer::singleShot(400, this, [this] { sendSearch(); });
    QTimer::singleShot(900, this, [this] { sendSearch(); });
}

void SsdpDiscovery::sendSearch()
{
    const QByteArray msg =
        "M-SEARCH * HTTP/1.1\r\n"
        "HOST: 239.255.255.250:1900\r\n"
        "MAN: \"ssdp:discover\"\r\n"
        "MX: 3\r\n"
        "ST: ssdp:all\r\n"
        "\r\n";
    m_socket.writeDatagram(msg, QHostAddress(kMulticastAddr), kMulticastPort);
}

void SsdpDiscovery::onReadyRead()
{
    while (m_socket.hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(m_socket.pendingDatagramSize());
        m_socket.readDatagram(datagram.data(), datagram.size());

        const QString text = QString::fromUtf8(datagram);
        QString location;
        QString usn;
        const QStringList lines = text.split(QStringLiteral("\r\n"));
        for (const QString &line : lines) {
            const int idx = line.indexOf(QLatin1Char(':'));
            if (idx < 0)
                continue;
            const QString key = line.left(idx).trimmed().toUpper();
            const QString value = line.mid(idx + 1).trimmed();
            if (key == QLatin1String("LOCATION"))
                location = value;
            else if (key == QLatin1String("USN"))
                usn = value;
        }

        if (location.isEmpty() || usn.isEmpty() || m_seenUsn.contains(usn))
            continue;
        m_seenUsn.insert(usn);
        emit deviceLocated(QUrl(location), usn);
    }
}