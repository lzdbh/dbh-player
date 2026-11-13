#include "SubtitleTrack.h"

#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStringDecoder>
#include <QTextCodec>

SubtitleTrack::SubtitleTrack(QObject *parent)
    : QObject(parent)
{
}

bool SubtitleTrack::loaded() const { return !m_cues.isEmpty(); }
QString SubtitleTrack::fileName() const { return m_fileName; }
int SubtitleTrack::cueCount() const { return m_cues.size(); }

bool SubtitleTrack::load(const QUrl &fileUrl)
{
    QFile file(fileUrl.toLocalFile());
    if (!file.open(QIODevice::ReadOnly))
        return false;
    const bool ok = parse(file.readAll());
    if (ok) {
        m_fileName = QFileInfo(file).fileName();
        emit loadedChanged();
    }
    return ok;
}

bool SubtitleTrack::autoLoadFor(const QUrl &videoUrl)
{
    const QFileInfo info(videoUrl.toLocalFile());
    const QString srtPath = info.absolutePath() + QLatin1Char('/')
                            + info.completeBaseName() + QLatin1String(".srt");
    if (!QFileInfo::exists(srtPath))
        return false;
    return load(QUrl::fromLocalFile(srtPath));
}

void SubtitleTrack::unload()
{
    if (m_cues.isEmpty() && m_fileName.isEmpty())
        return;
    m_cues.clear();
    m_fileName.clear();
    emit loadedChanged();
}

QString SubtitleTrack::textAt(qint64 positionMs) const
{
    // Cues are sorted by start time; binary search for the last cue starting
    // at or before positionMs, then check its end.
    int lo = 0, hi = m_cues.size() - 1, found = -1;
    while (lo <= hi) {
        const int mid = (lo + hi) / 2;
        if (m_cues.at(mid).startMs <= positionMs) {
            found = mid;
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }
    if (found >= 0 && positionMs < m_cues.at(found).endMs)
        return m_cues.at(found).text;
    return {};
}

bool SubtitleTrack::parse(const QByteArray &data)
{
    QString text;
    if (data.startsWith("\xef\xbb\xbf") || data.startsWith("\xff\xfe")
        || data.startsWith("\xfe\xff")) {
        auto decoder = QStringDecoder(QStringDecoder::Utf8);
        if (data.startsWith("\xff\xfe") || data.startsWith("\xfe\xff"))
            decoder = QStringDecoder(QStringDecoder::Utf16);
        text = decoder(data);
    } else {
        auto utf8 = QStringDecoder(QStringDecoder::Utf8);
        text = utf8(data);
        if (utf8.hasError()) {
            // Chinese SRT files are often GB18030; QStringDecoder lacks it
            // without ICU, so use the Core5Compat codec.
            if (auto *codec = QTextCodec::codecForName("GB18030"))
                text = codec->toUnicode(data);
        }
    }
    if (text.isEmpty())
        return false;

    static const QRegularExpression timeRe(
        QStringLiteral("(\\d{1,2}):(\\d{2}):(\\d{2})[,.](\\d{1,3})\\s*-->\\s*"
                       "(\\d{1,2}):(\\d{2}):(\\d{2})[,.](\\d{1,3})"));
    static const QRegularExpression tagRe(QStringLiteral("<[^>]+>|\\{[^}]*\\}"));

    QList<Cue> cues;
    const QStringList blocks = text.split(QRegularExpression(QStringLiteral("\\r?\\n\\s*\\r?\\n")),
                                          Qt::SkipEmptyParts);
    for (const QString &block : blocks) {
        const QStringList lines = block.split(QRegularExpression(QStringLiteral("\\r?\\n")));
        int timeLine = -1;
        for (int i = 0; i < lines.size(); ++i) {
            if (timeRe.match(lines.at(i)).hasMatch()) {
                timeLine = i;
                break;
            }
        }
        if (timeLine < 0 || timeLine + 1 >= lines.size())
            continue;
        const auto m = timeRe.match(lines.at(timeLine));
        const qint64 start = ((m.captured(1).toInt() * 3600 + m.captured(2).toInt() * 60
                               + m.captured(3).toInt()) * 1000)
                             + m.captured(4).leftJustified(3, QLatin1Char('0')).toInt();
        const qint64 end = ((m.captured(5).toInt() * 3600 + m.captured(6).toInt() * 60
                             + m.captured(7).toInt()) * 1000)
                           + m.captured(8).leftJustified(3, QLatin1Char('0')).toInt();
        QString cueText = lines.mid(timeLine + 1).join(QLatin1Char('\n'));
        cueText.remove(tagRe);
        cueText = cueText.trimmed();
        if (!cueText.isEmpty() && end > start)
            cues.append({start, end, cueText});
    }

    if (cues.isEmpty())
        return false;
    std::sort(cues.begin(), cues.end(),
              [](const Cue &a, const Cue &b) { return a.startMs < b.startMs; });
    m_cues = cues;
    return true;
}