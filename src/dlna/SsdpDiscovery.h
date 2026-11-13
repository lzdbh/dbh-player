#pragma once

#include <QObject>
#include <QSet>
#include <QUdpSocket>
#include <QUrl>

// SSDP control point: multicasts M-SEARCH and collects device advertisements.
// Emits deviceLocated for each unique USN seen (deduplicated here).
class SsdpDiscovery : public QObject
{
    Q_OBJECT

public:
    explicit SsdpDiscovery(QObject *parent = nullptr);

    void startSearch();

signals:
    void deviceLocated(const QUrl &location, const QString &usn);

private slots:
    void onReadyRead();

private:
    void sendSearch();

    QUdpSocket m_socket;
    QSet<QString> m_seenUsn;
    bool m_bound = false;
};