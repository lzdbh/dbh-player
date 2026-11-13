#include <QtTest>

#include "PlaylistController.h"
#include "SubtitleTrack.h"

class PlayerTest : public QObject
{
    Q_OBJECT

private slots:
    void srtParseAndLookup();
    void srtEncodingGb18030();
    void srtAutoLoad();
    void playlistAdvanceAndRemove();
};

void PlayerTest::srtParseAndLookup()
{
    QTemporaryFile file;
    QVERIFY(file.open());
    file.write("1\n00:00:01,000 --> 00:00:02,500\n<i>第一句</i>\n\n"
               "2\n00:00:02,600 --> 00:00:04,000\n第二句\n多行\n\n");
    file.flush();

    SubtitleTrack track;
    QVERIFY(track.load(QUrl::fromLocalFile(file.fileName())));
    QCOMPARE(track.cueCount(), 2);
    QCOMPARE(track.textAt(500), QString());
    QCOMPARE(track.textAt(1000), QStringLiteral("第一句"));
    QCOMPARE(track.textAt(2499), QStringLiteral("第一句"));
    QCOMPARE(track.textAt(2500), QString());
    QCOMPARE(track.textAt(3000), QStringLiteral("第二句\n多行"));
    QCOMPARE(track.textAt(4000), QString());
    track.unload();
    QVERIFY(!track.loaded());
}

void PlayerTest::srtEncodingGb18030()
{
    QTemporaryFile file;
    QVERIFY(file.open());
    // "你好" in GB18030
    const QByteArray gb = QByteArray::fromHex("c4e3bac3");
    file.write("1\n00:00:00,000 --> 00:00:01,000\n" + gb + "\n");
    file.flush();

    SubtitleTrack track;
    QVERIFY(track.load(QUrl::fromLocalFile(file.fileName())));
    QCOMPARE(track.textAt(500), QStringLiteral("你好"));
}

void PlayerTest::srtAutoLoad()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString video = dir.filePath("movie.mp4");
    const QString srt = dir.filePath("movie.srt");
    QFile vf(video);
    QVERIFY(vf.open(QIODevice::WriteOnly));
    vf.write("x");
    vf.close();
    QFile sf(srt);
    QVERIFY(sf.open(QIODevice::WriteOnly));
    sf.write("1\n00:00:00,000 --> 00:00:01,000\nauto\n");
    sf.close();

    SubtitleTrack track;
    QVERIFY(track.autoLoadFor(QUrl::fromLocalFile(video)));
    QCOMPARE(track.textAt(0), QStringLiteral("auto"));

    SubtitleTrack none;
    QVERIFY(!none.autoLoadFor(QUrl::fromLocalFile(dir.filePath("other.mp4"))));
}

void PlayerTest::playlistAdvanceAndRemove()
{
    PlaylistController list;
    QSignalSpy openSpy(&list, &PlaylistController::requestOpen);
    const QList<QUrl> urls = {QUrl("file:///a.mp4"), QUrl("file:///b.mp4"), QUrl("file:///c.mp4")};
    list.addUrls(urls);
    QCOMPARE(list.count(), 3);
    QCOMPARE(list.currentIndex(), 0);
    QCOMPARE(openSpy.count(), 1);

    list.onMediaEnded();
    QCOMPARE(list.currentIndex(), 1);
    list.next();
    QCOMPARE(list.currentIndex(), 2);
    list.onMediaEnded(); // at the end: stays
    QCOMPARE(list.currentIndex(), 2);
    list.previous();
    QCOMPARE(list.currentIndex(), 1);

    list.removeAt(0); // before current -> index shifts
    QCOMPARE(list.count(), 2);
    QCOMPARE(list.currentIndex(), 0);

    list.removeAt(0); // removes current -> plays next
    QCOMPARE(list.count(), 1);
    QCOMPARE(list.currentIndex(), 0);

    list.clear();
    QCOMPARE(list.count(), 0);
    QCOMPARE(list.currentIndex(), -1);
}

QTEST_MAIN(PlayerTest)
#include "playertest.moc"