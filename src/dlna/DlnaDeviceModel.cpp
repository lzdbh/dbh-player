#include "DlnaDeviceModel.h"

DlnaDeviceModel::DlnaDeviceModel(QObject *parent)
    : QAbstractListModel(parent)
{
    qRegisterMetaType<DlnaDeviceInfo>();
    connect(&m_discovery, &SsdpDiscovery::deviceLocated, this, &DlnaDeviceModel::onDeviceLocated);
    connect(&m_parser, &DeviceParser::deviceParsed, this, &DlnaDeviceModel::onDeviceParsed);
}

int DlnaDeviceModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_devices.size();
}

QVariant DlnaDeviceModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_devices.size())
        return {};

    const DlnaDeviceInfo &info = m_devices.at(index.row());
    switch (role) {
    case NameRole:
        return info.friendlyName;
    case TypeRole:
        return info.deviceType;
    case ManufacturerRole:
        return info.manufacturer;
    case LocationRole:
        return info.location;
    case AvTransportRole:
        return info.avTransportControlUrl;
    case RenderingControlRole:
        return info.renderingControlUrl;
    case UsnRole:
        return info.usn;
    default:
        return {};
    }
}

QHash<int, QByteArray> DlnaDeviceModel::roleNames() const
{
    return {
        { NameRole, "name" },
        { TypeRole, "type" },
        { ManufacturerRole, "manufacturer" },
        { LocationRole, "location" },
        { AvTransportRole, "avTransport" },
        { RenderingControlRole, "renderingControl" },
        { UsnRole, "usn" },
    };
}

void DlnaDeviceModel::scan()
{
    m_discovery.startSearch();
}

void DlnaDeviceModel::clear()
{
    if (m_devices.isEmpty())
        return;
    beginResetModel();
    m_devices.clear();
    m_knownUuid.clear();
    endResetModel();
}

void DlnaDeviceModel::onDeviceLocated(const QUrl &location, const QString &usn)
{
    const QString uuid = usn.section(QStringLiteral("::"), 0, 0);
    if (m_knownUuid.contains(uuid) || m_pendingUuid.contains(uuid))
        return;
    m_pendingUuid.insert(uuid);
    m_parser.fetch(location, usn);
}

void DlnaDeviceModel::onDeviceParsed(const DlnaDeviceInfo &info)
{
    const QString uuid = info.usn.section(QStringLiteral("::"), 0, 0);
    m_pendingUuid.remove(uuid);

    // Only keep MediaRenderers (the TVs / cast receivers we can push to).
    if (!info.deviceType.contains(QLatin1String("MediaRenderer")) || m_knownUuid.contains(uuid))
        return;

    m_knownUuid.insert(uuid);
    beginInsertRows(QModelIndex(), m_devices.size(), m_devices.size());
    m_devices.append(info);
    endInsertRows();
}