#include <QtTest>

#include "AvTransportClient.h"
#include "MediaHttpServer.h"
#include "SoapClient.h"
#include "TranscodeSession.h"

#include <QProcess>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>

class DlnaTest : public QObject
{
    Q_OBJECT

private slots:
    void timeFormatParse();
    void xmlEscape();
    void soapSuccessAndFault();
    void httpRangeRequests();
    void liveTranscodeStream();
};

void DlnaTest::timeFormatParse()
{
    QCOMPARE(AvTransportClient::formatTime(0), QStringLiteral("00:00:00"));
    QCOMPARE(AvTransportClient::formatTime(3661000), QStringLiteral("01:01:01"));
    QCOMPARE(AvTransportClient::parseTime("01:01:01"), 3661000);
    QCOMPARE(AvTransportClient::parseTime("00:00:02.500"), 2500);
    QCOMPARE(AvTransportClient::parseTime("NOT_IMPLEMENTED"), 0);
    QCOMPARE(AvTransportClient::parseTime(""), 0);
}

void DlnaTest::xmlEscape()
{
    QCOMPARE(SoapClient::xmlEscape("a&b<c>d\"e'f"),
             QStringLiteral("a&amp;b&lt;c&gt;d&quot;e&apos;f"));
}

void DlnaTest::soapSuccessAndFault()
{
    // Minimal canned SOAP server.
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
    connect(&server, &QTcpServer::newConnection, &server, [&server]() {
        auto *sock = server.nextPendingConnection();
        connect(sock, &QTcpSocket::readyRead, sock, [sock]() {
            const QByteArray req = sock->readAll();
            QByteArray body;
            if (req.contains("GetPositionInfo")) {
                body = "<u:GetPositionInfoResponse xmlns:u=\"urn:schemas-upnp-org:service:AVTransport:1\">"
                       "<RelTime>00:00:03</RelTime>"
                       "<TrackDuration>00:00:05</TrackDuration></u:GetPositionInfoResponse>";
            } else {
                body = "<s:Fault><faultcode>s:Client</faultcode>"
                       "<faultstring>Invalid Action</faultstring></s:Fault>";
            }
            const QByteArray xml = "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\">"
                                   "<s:Body>" + body + "</s:Body></s:Envelope>";
            sock->write("HTTP/1.1 200 OK\r\nContent-Length: " + QByteArray::number(xml.size())
                        + "\r\nConnection: close\r\n\r\n" + xml);
            sock->disconnectFromHost();
        });
    });

    SoapClient client;
    const QUrl url(QStringLiteral("http://127.0.0.1:%1/ctl").arg(server.serverPort()));

    bool okCalled = false;
    client.sendAction(url, "urn:schemas-upnp-org:service:AVTransport:1", "GetPositionInfo",
                      {{"InstanceID", "0"}},
                      [&](bool ok, const SoapClient::Result &values, const QString &error) {
                          okCalled = true;
                          if (!ok)
                              qWarning() << "soap error:" << error;
                          QVERIFY(ok);
                          QCOMPARE(values.value("RelTime"), QStringLiteral("00:00:03"));
                          QCOMPARE(values.value("TrackDuration"), QStringLiteral("00:00:05"));
                      });
    QTRY_VERIFY_WITH_TIMEOUT(okCalled, 3000);

    bool faultCalled = false;
    client.sendAction(url, "urn:schemas-upnp-org:service:AVTransport:1", "Bogus", {},
                      [&](bool ok, const SoapClient::Result &, const QString &error) {
                          faultCalled = true;
                          QVERIFY(!ok);
                          QVERIFY(error.contains("fault"));
                      });
    QTRY_VERIFY_WITH_TIMEOUT(faultCalled, 3000);
}

void DlnaTest::httpRangeRequests()
{
    QTemporaryFile file;
    QVERIFY(file.open());
    QByteArray data(10000, '\0');
    for (int i = 0; i < data.size(); ++i)
        data[i] = char(i % 251);
    file.write(data);
    file.flush();

    MediaHttpServer server;
    QVERIFY(server.start(file.fileName()));
    const quint16 port = server.serverPort();
    QCOMPARE(server.fileSize(), 10000);

    // The server side needs the event loop pumped while the client socket
    // blocks, otherwise QTcpServer never accepts the connection.
    auto request = [port](const QByteArray &req) -> QByteArray {
        QTcpSocket sock;
        sock.connectToHost(QHostAddress::LocalHost, port);
        QElapsedTimer timer;
        timer.start();
        while (sock.state() != QAbstractSocket::ConnectedState && !timer.hasExpired(2000))
            QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
        if (sock.state() != QAbstractSocket::ConnectedState)
            return {};
        sock.write(req);
        QByteArray response;
        timer.restart();
        while (!timer.hasExpired(3000)) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
            response += sock.readAll();
            if (sock.state() == QAbstractSocket::UnconnectedState) {
                response += sock.readAll();
                break;
            }
        }
        return response;
    };

    const QByteArray full = request("GET /media HTTP/1.1\r\nHost: x\r\n\r\n");
    QVERIFY(full.startsWith("HTTP/1.1 200 OK"));
    QVERIFY(full.contains("Content-Length: 10000"));
    QCOMPARE(full.size() - full.indexOf("\r\n\r\n") - 4, 10000);

    const QByteArray part = request("GET /media HTTP/1.1\r\nRange: bytes=100-199\r\n\r\n");
    QVERIFY(part.startsWith("HTTP/1.1 206"));
    QVERIFY(part.contains("Content-Range: bytes 100-199/10000"));
    const QByteArray body = part.mid(part.indexOf("\r\n\r\n") + 4);
    QCOMPARE(body.size(), 100);
    QCOMPARE(body, data.mid(100, 100));

    const QByteArray suffix = request("GET /media HTTP/1.1\r\nRange: bytes=-50\r\n\r\n");
    QVERIFY(suffix.startsWith("HTTP/1.1 206"));
    QCOMPARE(suffix.mid(suffix.indexOf("\r\n\r\n") + 4), data.right(50));

    const QByteArray bad = request("GET /media HTTP/1.1\r\nRange: bytes=99999-\r\n\r\n");
    QVERIFY(bad.startsWith("HTTP/1.1 416"));

    const QByteArray head = request("HEAD /media HTTP/1.1\r\n\r\n");
    QVERIFY(head.startsWith("HTTP/1.1 200"));
    QVERIFY(head.contains("Accept-Ranges: bytes"));
    QCOMPARE(head.mid(head.indexOf("\r\n\r\n") + 4).size(), 0);

    const QByteArray missing = request("GET /nope HTTP/1.1\r\n\r\n");
    QVERIFY(missing.startsWith("HTTP/1.1 404"));
}

void DlnaTest::liveTranscodeStream()
{
    const QString ffmpeg = TranscodeSession::ffmpegPath();
    if (ffmpeg.isEmpty())
        QSKIP("ffmpeg not installed");

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString video = dir.filePath("src.mp4");
    QProcess gen;
    gen.start(ffmpeg, {QStringLiteral("-y"), QStringLiteral("-f"), QStringLiteral("lavfi"),
                       QStringLiteral("-i"), QStringLiteral("testsrc=duration=6:size=640x360:rate=30"),
                       QStringLiteral("-f"), QStringLiteral("lavfi"),
                       QStringLiteral("-i"), QStringLiteral("sine=frequency=440:duration=6"),
                       QStringLiteral("-c:v"), QStringLiteral("mpeg4"), QStringLiteral("-b:v"), QStringLiteral("2M"),
                       QStringLiteral("-pix_fmt"), QStringLiteral("yuv420p"),
                       QStringLiteral("-c:a"), QStringLiteral("aac"), QStringLiteral("-shortest"), video});
    QVERIFY(gen.waitForFinished(15000));
    QCOMPARE(gen.exitCode(), 0);

    TranscodeSession session;
    QVERIFY(session.start(video));
    MediaHttpServer server;
    QVERIFY(server.startLive(session.stream(), QStringLiteral("video/mp2t")));

    QTcpSocket client;
    client.connectToHost(QHostAddress::LocalHost, server.serverPort());
    QVERIFY(client.waitForConnected(2000));
    client.write("GET /media HTTP/1.1\r\nHost: x\r\n\r\n");

    // Consume ~160KB/s, slower than the output stream, so TCP backpressure
    // blocks ffmpeg. A fast producer may finish and close within the window;
    // that is a valid EOF, but closing while ffmpeg still runs is the
    // premature-EOF bug (empty read buffer mistaken for end of stream).
    QByteArray received;
    QElapsedTimer total;
    total.start();
    QElapsedTimer sinceRead;
    sinceRead.start();
    while (!total.hasExpired(3000)) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
        if (sinceRead.elapsed() >= 100 && client.bytesAvailable() > 0) {
            received += client.read(16 * 1024);
            sinceRead.restart();
        }
        if (client.state() != QAbstractSocket::ConnectedState) {
            // EOF on the producer stream must precede closing the client;
            // the process may still be exiting (state reaped a tick later).
            auto *streamSock = qobject_cast<QTcpSocket *>(session.stream());
            QVERIFY(streamSock);
            QCOMPARE(streamSock->state(), QAbstractSocket::UnconnectedState);
            break;
        }
    }
    if (client.state() == QAbstractSocket::UnconnectedState)
        received += client.readAll();
    QVERIFY(received.startsWith("HTTP/1.1 200 OK"));
    QVERIFY(received.contains("video/mp2t"));
    const qsizetype bodyStart = received.indexOf("\r\n\r\n") + 4;
    QVERIFY(bodyStart > 4);
    QCOMPARE(quint8(received.at(bodyStart)), quint8(0x47)); // TS sync byte
    QVERIFY(received.size() > 100 * 1024);

    session.stop();
    QElapsedTimer drain;
    drain.start();
    while (client.state() != QAbstractSocket::UnconnectedState && !drain.hasExpired(5000)) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
        client.readAll();
    }
    QCOMPARE(client.state(), QAbstractSocket::UnconnectedState);
}

QTEST_MAIN(DlnaTest)
#include "dlnatest.moc"