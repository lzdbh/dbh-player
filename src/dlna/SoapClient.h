#pragma once

#include <QNetworkAccessManager>
#include <QObject>
#include <QUrl>

#include <functional>

// Minimal UPnP SOAP client: POSTs an action envelope and flattens the
// response's leaf elements into a name->text map. A SOAP Fault is reported
// as an error.
class SoapClient : public QObject
{
    Q_OBJECT

public:
    using Result = QMap<QString, QString>;
    using Callback = std::function<void(bool ok, const Result &values, const QString &error)>;

    explicit SoapClient(QObject *parent = nullptr);

    void sendAction(const QUrl &controlUrl,
                    const QString &serviceType,
                    const QString &action,
                    const QList<QPair<QString, QString>> &args,
                    Callback callback);

    static QString xmlEscape(const QString &text);

private:
    QNetworkAccessManager m_nam;
};