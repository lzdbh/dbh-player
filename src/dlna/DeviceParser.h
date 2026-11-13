#pragma once

#include <QNetworkAccessManager>
#include <QObject>
#include <QUrl>

// Parsed UPnP device description (only the fields needed for casting).
struct DlnaDeviceInfo
{
    QString friendlyName;
    QString deviceType;
    QString manufacturer;
    QString modelName;
    QString usn;
    QUrl location;
    QUrl avTransportControlUrl;
    QUrl renderingControlUrl;
};

Q_DECLARE_METATYPE(DlnaDeviceInfo)

// Fetches a device description XML and parses it into DlnaDeviceInfo.
class DeviceParser : public QObject
{
    Q_OBJECT

public:
    explicit DeviceParser(QObject *parent = nullptr);

    void fetch(const QUrl &location, const QString &usn);

signals:
    void deviceParsed(const DlnaDeviceInfo &info);
    void parseFailed(const QString &usn, const QString &error);

private:
    QNetworkAccessManager m_nam;
};