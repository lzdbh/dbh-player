#pragma once

#include "SoapClient.h"

#include <QObject>

// SOAP client for the UPnP RenderingControl service (volume/mute).
class RenderingControlClient : public QObject
{
    Q_OBJECT

public:
    explicit RenderingControlClient(QObject *parent = nullptr);

    void setControlUrl(const QUrl &url);

    void setVolume(int volume); // 0..100
    void getVolume();
    void setMute(bool mute);

signals:
    void volumeReceived(int volume);
    void actionFailed(const QString &action, const QString &error);

private:
    SoapClient m_soap;
    QUrl m_controlUrl;
};