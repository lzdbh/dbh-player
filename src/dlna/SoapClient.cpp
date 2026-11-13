#include "SoapClient.h"

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QXmlStreamReader>

SoapClient::SoapClient(QObject *parent)
    : QObject(parent)
{
}

QString SoapClient::xmlEscape(const QString &text)
{
    QString out;
    out.reserve(text.size());
    for (const QChar c : text) {
        switch (c.unicode()) {
        case u'&': out += QLatin1String("&amp;"); break;
        case u'<': out += QLatin1String("&lt;"); break;
        case u'>': out += QLatin1String("&gt;"); break;
        case u'"': out += QLatin1String("&quot;"); break;
        case u'\'': out += QLatin1String("&apos;"); break;
        default: out += c;
        }
    }
    return out;
}

void SoapClient::sendAction(const QUrl &controlUrl,
                            const QString &serviceType,
                            const QString &action,
                            const QList<QPair<QString, QString>> &args,
                            Callback callback)
{
    QByteArray body;
    body += "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
            "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
            "s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\"><s:Body>";
    body += "<u:" + action.toUtf8() + " xmlns:u=\"" + serviceType.toUtf8() + "\">";
    for (const auto &arg : args)
        body += "<" + arg.first.toUtf8() + ">" + xmlEscape(arg.second).toUtf8()
                + "</" + arg.first.toUtf8() + ">";
    body += "</u:" + action.toUtf8() + "></s:Body></s:Envelope>";

    QNetworkRequest request(controlUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QLatin1String("text/xml; charset=\"utf-8\""));
    request.setRawHeader(QByteArrayLiteral("SOAPAction"),
                         "\"" + serviceType.toUtf8() + "#" + action.toUtf8() + "\"");
    request.setTransferTimeout(8000);

    QNetworkReply *reply = m_nam.post(request, body);
    connect(reply, &QNetworkReply::finished, this, [reply, action, callback]() {
        reply->deleteLater();
        const QByteArray payload = reply->readAll();

        if (reply->error() != QNetworkReply::NoError) {
            callback(false, {}, QStringLiteral("%1: %2").arg(action, reply->errorString()));
            return;
        }

        Result values;
        QString fault;
        QString currentName;
        QXmlStreamReader reader(payload);
        while (!reader.atEnd() && !reader.hasError()) {
            reader.readNext();
            if (reader.isStartElement()) {
                currentName = reader.name().toString();
                if (currentName == QLatin1String("Fault")) {
                    fault = reader.readElementText(QXmlStreamReader::IncludeChildElements);
                    break;
                }
            } else if (reader.isCharacters() && !currentName.isEmpty()) {
                values.insert(currentName, reader.text().toString());
            } else if (reader.isEndElement()) {
                currentName.clear();
            }
        }

        if (!fault.isEmpty()) {
            callback(false, {}, QStringLiteral("%1: SOAP fault %2").arg(action, fault));
            return;
        }
        if (reader.hasError()) {
            callback(false, {}, QStringLiteral("%1: bad XML response").arg(action));
            return;
        }
        callback(true, values, {});
    });
}