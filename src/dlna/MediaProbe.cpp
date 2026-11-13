#include "MediaProbe.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QStandardPaths>

namespace {
QString findTool(const QString &name)
{
    const QString found = QStandardPaths::findExecutable(name);
    if (!found.isEmpty())
        return found;
    const QStringList extra = {
        QStringLiteral("/usr/local/bin/%1").arg(name),
        QStringLiteral("/opt/homebrew/bin/%1").arg(name),
    };
    for (const QString &path : extra) {
        if (QFile::exists(path))
            return path;
    }
    return {};
}
} // namespace

MediaProbe::MediaProbe(QObject *parent)
    : QObject(parent)
{
}

QString MediaProbe::ffprobePath()
{
    return findTool(QStringLiteral("ffprobe"));
}

void MediaProbe::probe(const QString &filePath)
{
    const QString tool = ffprobePath();
    if (tool.isEmpty()) {
        Result r;
        r.error = tr("ffprobe not found; install ffmpeg to enable format detection.");
        emit probed(r);
        return;
    }

    auto *process = new QProcess(this);
    connect(process, &QProcess::finished, this, [this, process](int exitCode) {
        process->deleteLater();
        Result r;
        if (exitCode != 0) {
            r.error = tr("ffprobe failed: %1")
                          .arg(QString::fromUtf8(process->readAllStandardError()).left(200));
            emit probed(r);
            return;
        }

        const QJsonDocument doc = QJsonDocument::fromJson(process->readAllStandardOutput());
        const QJsonObject root = doc.object();
        r.valid = true;
        r.formatName = root.value(QLatin1String("format")).toObject()
                           .value(QLatin1String("format_name")).toString();
        r.durationMs = qint64(root.value(QLatin1String("format")).toObject()
                                  .value(QLatin1String("duration")).toString().toDouble() * 1000);

        const QJsonArray streams = root.value(QLatin1String("streams")).toArray();
        for (const QJsonValue &v : streams) {
            const QJsonObject s = v.toObject();
            const QString type = s.value(QLatin1String("codec_type")).toString();
            if (type == QLatin1String("video") && r.videoCodec.isEmpty())
                r.videoCodec = s.value(QLatin1String("codec_name")).toString();
            else if (type == QLatin1String("audio") && r.audioCodec.isEmpty())
                r.audioCodec = s.value(QLatin1String("codec_name")).toString();
        }

        const bool containerOk = r.formatName.contains(QLatin1String("mp4"))
                                 || r.formatName.contains(QLatin1String("mov"));
        const bool videoOk = r.videoCodec == QLatin1String("h264");
        const bool audioOk = r.audioCodec.isEmpty()
                             || r.audioCodec == QLatin1String("aac")
                             || r.audioCodec == QLatin1String("mp3");
        r.directPushCompatible = containerOk && videoOk && audioOk;
        emit probed(r);
    });
    connect(process, &QProcess::errorOccurred, this, [this, process](QProcess::ProcessError) {
        process->deleteLater();
        Result r;
        r.error = tr("ffprobe could not start.");
        emit probed(r);
    });

    process->start(tool, {QStringLiteral("-v"), QStringLiteral("quiet"),
                          QStringLiteral("-print_format"), QStringLiteral("json"),
                          QStringLiteral("-show_streams"), QStringLiteral("-show_format"),
                          filePath});
}