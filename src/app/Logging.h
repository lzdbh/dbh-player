#pragma once

#include <QFile>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(lcPlayer)
Q_DECLARE_LOGGING_CATEGORY(lcDlna)
Q_DECLARE_LOGGING_CATEGORY(lcCast)
Q_DECLARE_LOGGING_CATEGORY(lcHttp)

namespace Logging {
// Installs a message handler that mirrors Qt logs to stderr and to a
// rotating file under AppDataLocation/logs (keeps the last 5 sessions).
void install();
}