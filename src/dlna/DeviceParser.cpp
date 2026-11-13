#include "DeviceParser.h"

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QXmlStreamReader>

DeviceParser::DeviceParser(QObject *parent)
    : QObject(parent)
{
}

void DeviceParser::fetch(const QUrl &location, const QString &usn)
{
    QNetworkReply *reply = m_nam.get(QNetworkRequest(location));
    connect(reply, &QNetworkReply::finished, this, [this, reply, location, usn]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit parseFailed(usn, reply->errorString());
            return;
        }

        DlnaDeviceInfo info;
        info.location = location;
        info.usn = usn;

        QXmlStreamReader reader(reply->readAll());
        bool inService = false;
        QString serviceType;
        QString controlUrl;

        while (!reader.atEnd() && !reader.hasError()) {
            reader.readNext();
            if (reader.isStartElement()) {
                const QString name = reader.name().toString();
                if (name == QLatin1String("friendlyName") && info.friendlyName.isEmpty())
                    info.friendlyName = reader.readElementText();
                else if (name == QLatin1String("deviceType") && info.deviceType.isEmpty())
                    info.deviceType = reader.readElementText();
                else if (name == QLatin1String("manufacturer") && info.manufacturer.isEmpty())
                    info.manufacturer = reader.readElementText();
                else if (name == QLatin1String("modelName") && info.modelName.isEmpty())
                    info.modelName = reader.readElementText();
                else if (name == QLatin1String("service")) {
                    inService = true;
                    serviceType.clear();
                    controlUrl.clear();
                } else if (inService && name == QLatin1String("serviceType"))
                    serviceType = reader.readElementText();
                else if (inService && name == QLatin1String("controlURL"))
                    controlUrl = reader.readElementText();
            } else if (reader.isEndElement()) {
                if (reader.name().toString() == QLatin1String("service")) {
                    if (!controlUrl.isEmpty()) {
                        const QUrl resolved = location.resolved(QUrl(controlUrl));
                        if (serviceType.contains(QLatin1String("AVTransport")))
                            info.avTransportControlUrl = resolved;
                        else if (serviceType.contains(QLatin1String("RenderingControl")))
                            info.renderingControlUrl = resolved;
                    }
                    inService = false;
                }
            }
        }

        if (info.friendlyName.isEmpty())
            info.friendlyName = location.host();
        emit deviceParsed(info);
    });
}