#include "Logging.h"

#include <QDateTime>
#include <QDir>
#include <QMutex>
#include <QMutexLocker>
#include <QStandardPaths>
#include <QTextStream>

Q_LOGGING_CATEGORY(lcPlayer, "dbhx.player")
Q_LOGGING_CATEGORY(lcDlna, "dbhx.dlna")
Q_LOGGING_CATEGORY(lcCast, "dbhx.cast")
Q_LOGGING_CATEGORY(lcHttp, "dbhx.http")

namespace {
QFile *logFile()
{
    static QFile *file = []() {
        const QString dirPath =
            QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
            + QLatin1String("/logs");
        QDir dir(dirPath);
        dir.mkpath(QStringLiteral("."));

        // Rotate: keep the 5 newest session logs.
        const QStringList old = dir.entryList({QStringLiteral("dbhx-*.log")},
                                              QDir::Files, QDir::Time);
        for (int i = 4; i < old.size(); ++i)
            dir.remove(old.at(i));

        auto *f = new QFile(dirPath + QLatin1String("/dbhx-")
                            + QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-hhmmss"))
                            + QLatin1String(".log"));
        f->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
        return f;
    }();
    return file;
}

void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    static QMutex mutex;
    QMutexLocker lock(&mutex);

    const char *level = type == QtDebugMsg ? "DEBUG"
                        : type == QtInfoMsg ? "INFO"
                        : type == QtWarningMsg ? "WARN"
                        : type == QtCriticalMsg ? "ERROR" : "FATAL";
    const QString line = QStringLiteral("%1 [%2] %3: %4\n")
                             .arg(QDateTime::currentDateTime().toString(Qt::ISODateWithMs),
                                  QLatin1String(level),
                                  QLatin1String(context.category ? context.category : "default"),
                                  message);

    QTextStream(stderr) << line;
    QFile *file = logFile();
    if (file->isOpen()) {
        file->write(line.toUtf8());
        file->flush();
    }
}
} // namespace

void Logging::install()
{
    qInstallMessageHandler(messageHandler);
}