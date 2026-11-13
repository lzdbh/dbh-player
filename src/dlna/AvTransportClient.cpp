#include "AvTransportClient.h"

namespace {
const QString kServiceType = QStringLiteral("urn:schemas-upnp-org:service:AVTransport:1");

QString didlMetadata(const QUrl &mediaUrl, const QString &title,
                     const QString &mimeType, qint64 sizeBytes)
{
    const QString url = SoapClient::xmlEscape(mediaUrl.toString());
    const QString protocolInfo =
        QStringLiteral("http-get:*:%1:DLNA.ORG_OP=01;DLNA.ORG_CI=0;"
                       "DLNA.ORG_FLAGS=01700000000000000000000000000000").arg(mimeType);
    QString didl;
    didl += QLatin1String("<DIDL-Lite xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\" "
                          "xmlns:dc=\"http://purl.org/dc/elements/1.1/\" "
                          "xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\">"
                          "<item id=\"0\" parentID=\"0\" restricted=\"1\">");
    didl += QLatin1String("<dc:title>") + SoapClient::xmlEscape(title) + QLatin1String("</dc:title>");
    didl += QLatin1String("<upnp:class>object.item.videoItem</upnp:class>");
    didl += QLatin1String("<res protocolInfo=\"") + protocolInfo + QLatin1String("\"");
    if (sizeBytes > 0)
        didl += QLatin1String(" size=\"") + QString::number(sizeBytes) + QLatin1String("\"");
    didl += QLatin1String(">") + url + QLatin1String("</res></item></DIDL-Lite>");
    return didl;
}
} // namespace

AvTransportClient::AvTransportClient(QObject *parent)
    : QObject(parent)
{
}

void AvTransportClient::setControlUrl(const QUrl &url)
{
    m_controlUrl = url;
}

void AvTransportClient::setUri(const QUrl &mediaUrl, const QString &title,
                               const QString &mimeType, qint64 sizeBytes)
{
    m_soap.sendAction(m_controlUrl, kServiceType, QStringLiteral("SetAVTransportURI"),
                      {{QStringLiteral("InstanceID"), QStringLiteral("0")},
                       {QStringLiteral("CurrentURI"), mediaUrl.toString()},
                       {QStringLiteral("CurrentURIMetaData"),
                        didlMetadata(mediaUrl, title, mimeType, sizeBytes)}},
                      [this](bool ok, const SoapClient::Result &, const QString &error) {
                          if (ok)
                              emit uriSet();
                          else
                              emit actionFailed(QStringLiteral("SetAVTransportURI"), error);
                      });
}

void AvTransportClient::play()
{
    m_soap.sendAction(m_controlUrl, kServiceType, QStringLiteral("Play"),
                      {{QStringLiteral("InstanceID"), QStringLiteral("0")},
                       {QStringLiteral("Speed"), QStringLiteral("1")}},
                      [this](bool ok, const SoapClient::Result &, const QString &error) {
                          if (!ok)
                              emit actionFailed(QStringLiteral("Play"), error);
                      });
}

void AvTransportClient::pause()
{
    m_soap.sendAction(m_controlUrl, kServiceType, QStringLiteral("Pause"),
                      {{QStringLiteral("InstanceID"), QStringLiteral("0")}},
                      [this](bool ok, const SoapClient::Result &, const QString &error) {
                          if (!ok)
                              emit actionFailed(QStringLiteral("Pause"), error);
                      });
}

void AvTransportClient::stop()
{
    m_soap.sendAction(m_controlUrl, kServiceType, QStringLiteral("Stop"),
                      {{QStringLiteral("InstanceID"), QStringLiteral("0")}},
                      [this](bool ok, const SoapClient::Result &, const QString &error) {
                          if (!ok)
                              emit actionFailed(QStringLiteral("Stop"), error);
                      });
}

void AvTransportClient::seek(qint64 positionMs)
{
    m_soap.sendAction(m_controlUrl, kServiceType, QStringLiteral("Seek"),
                      {{QStringLiteral("InstanceID"), QStringLiteral("0")},
                       {QStringLiteral("Unit"), QStringLiteral("REL_TIME")},
                       {QStringLiteral("Target"), formatTime(positionMs)}},
                      [this](bool ok, const SoapClient::Result &, const QString &error) {
                          if (!ok)
                              emit actionFailed(QStringLiteral("Seek"), error);
                      });
}

void AvTransportClient::getPositionInfo()
{
    m_soap.sendAction(m_controlUrl, kServiceType, QStringLiteral("GetPositionInfo"),
                      {{QStringLiteral("InstanceID"), QStringLiteral("0")}},
                      [this](bool ok, const SoapClient::Result &values, const QString &error) {
                          if (!ok) {
                              emit actionFailed(QStringLiteral("GetPositionInfo"), error);
                              return;
                          }
                          emit positionInfoReceived(
                              parseTime(values.value(QStringLiteral("RelTime"))),
                              parseTime(values.value(QStringLiteral("TrackDuration"))));
                      });
}

void AvTransportClient::getTransportInfo()
{
    m_soap.sendAction(m_controlUrl, kServiceType, QStringLiteral("GetTransportInfo"),
                      {{QStringLiteral("InstanceID"), QStringLiteral("0")}},
                      [this](bool ok, const SoapClient::Result &values, const QString &error) {
                          if (!ok) {
                              emit actionFailed(QStringLiteral("GetTransportInfo"), error);
                              return;
                          }
                          emit transportStateChanged(
                              values.value(QStringLiteral("CurrentTransportState")));
                      });
}

QString AvTransportClient::formatTime(qint64 ms)
{
    const qint64 totalSec = qMax<qint64>(0, ms / 1000);
    return QStringLiteral("%1:%2:%3")
        .arg(totalSec / 3600, 2, 10, QLatin1Char('0'))
        .arg((totalSec % 3600) / 60, 2, 10, QLatin1Char('0'))
        .arg(totalSec % 60, 2, 10, QLatin1Char('0'));
}

qint64 AvTransportClient::parseTime(const QString &hhmmss)
{
    const QStringList parts = hhmmss.split(QLatin1Char(':'));
    if (parts.size() != 3)
        return 0;
    bool okH = false, okM = false, okS = false;
    const qint64 h = parts.at(0).toLongLong(&okH);
    const qint64 m = parts.at(1).toLongLong(&okM);
    const double s = parts.at(2).toDouble(&okS);
    if (!okH || !okM || !okS)
        return 0;
    return ((h * 3600 + m * 60) * 1000) + qint64(s * 1000);
}