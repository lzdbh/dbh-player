#pragma once

#include "DeviceParser.h"
#include "SsdpDiscovery.h"

#include <QAbstractListModel>
#include <QList>
#include <QSet>
#include <QtQml/qqmlregistration.h>

// List of discovered DLNA MediaRenderers, exposed to QML as a list model.
class DlnaDeviceModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        TypeRole,
        ManufacturerRole,
        LocationRole,
        AvTransportRole,
        RenderingControlRole,
        UsnRole
    };

    explicit DlnaDeviceModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void scan();
    Q_INVOKABLE void clear();

private:
    void onDeviceLocated(const QUrl &location, const QString &usn);
    void onDeviceParsed(const DlnaDeviceInfo &info);

    SsdpDiscovery m_discovery;
    DeviceParser m_parser;
    QList<DlnaDeviceInfo> m_devices;
    QSet<QString> m_knownUuid;
    QSet<QString> m_pendingUuid;
};