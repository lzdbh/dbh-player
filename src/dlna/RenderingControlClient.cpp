#include "RenderingControlClient.h"

namespace {
const QString kServiceType = QStringLiteral("urn:schemas-upnp-org:service:RenderingControl:1");
}

RenderingControlClient::RenderingControlClient(QObject *parent)
    : QObject(parent)
{
}

void RenderingControlClient::setControlUrl(const QUrl &url)
{
    m_controlUrl = url;
}

void RenderingControlClient::setVolume(int volume)
{
    m_soap.sendAction(m_controlUrl, kServiceType, QStringLiteral("SetVolume"),
                      {{QStringLiteral("InstanceID"), QStringLiteral("0")},
                       {QStringLiteral("Channel"), QStringLiteral("Master")},
                       {QStringLiteral("DesiredVolume"),
                        QString::number(qBound(0, volume, 100))}},
                      [this](bool ok, const SoapClient::Result &, const QString &error) {
                          if (!ok)
                              emit actionFailed(QStringLiteral("SetVolume"), error);
                      });
}

void RenderingControlClient::getVolume()
{
    m_soap.sendAction(m_controlUrl, kServiceType, QStringLiteral("GetVolume"),
                      {{QStringLiteral("InstanceID"), QStringLiteral("0")},
                       {QStringLiteral("Channel"), QStringLiteral("Master")}},
                      [this](bool ok, const SoapClient::Result &values, const QString &error) {
                          if (!ok) {
                              emit actionFailed(QStringLiteral("GetVolume"), error);
                              return;
                          }
                          emit volumeReceived(
                              values.value(QStringLiteral("CurrentVolume")).toInt());
                      });
}

void RenderingControlClient::setMute(bool mute)
{
    m_soap.sendAction(m_controlUrl, kServiceType, QStringLiteral("SetMute"),
                      {{QStringLiteral("InstanceID"), QStringLiteral("0")},
                       {QStringLiteral("Channel"), QStringLiteral("Master")},
                       {QStringLiteral("DesiredMute"),
                        mute ? QStringLiteral("1") : QStringLiteral("0")}},
                      [this](bool ok, const SoapClient::Result &, const QString &error) {
                          if (!ok)
                              emit actionFailed(QStringLiteral("SetMute"), error);
                      });
}